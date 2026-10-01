#include "script/api_registry.h"

#include "event/event_bus.h"
#include "log/app_log.h"
#include "script/lua_engine.h"

#include <memory>
#include <sol/sol.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace api {
namespace {
// 绑定 api.log.*: 直接把 app_log 的日志函数暴露给 Lua。
void register_log(lua_State* L) {
    sol::state_view lua(L);
    sol::table api = lua["api"].get_or_create<sol::table>();
    sol::table log = api["log"].get_or_create<sol::table>();
    log["debug"] = &app_log::debug;
    log["info"] = &app_log::info;
    log["warn"] = &app_log::warn;
    log["error"] = &app_log::error;
    log["critical"] = &app_log::critical;
}

// 绑定 api.util.*: P0-04 返回值链路验证用最小演示 (临时, 后续按需替换/移除)。
void register_util(lua_State* L) {
    sol::state_view lua(L);
    sol::table api = lua["api"].get_or_create<sol::table>();
    sol::table util = api["util"].get_or_create<sol::table>();
    util["add"] = [](double a, double b) { return a + b; };
}

// event::Value → sol::object (载荷传给 Lua handler)。
sol::object to_sol_object(sol::state_view& lua, const event::Value& payload) {
    if (payload.is_number()) {
        return sol::make_object(lua, payload.as_number());
    }
    if (payload.is_string()) {
        return sol::make_object(lua, payload.as_string());
    }
    if (payload.is_bool()) {
        return sol::make_object(lua, payload.as_bool());
    }
    return sol::make_object(lua, sol::nil);
}

// sol::object → event::Value (Lua 侧 emit 的载荷)。
event::Value to_event_value(const sol::object& obj) {
    switch (obj.get_type()) {
    case sol::type::number:
        return event::Value::number(obj.as<double>());
    case sol::type::string:
        return event::Value::string(obj.as<std::string>());
    case sol::type::boolean:
        return event::Value::boolean(obj.as<bool>());
    case sol::type::nil:
    case sol::type::none:
    default:
        return event::Value::nil();
    }
}
} // namespace

void register_event(lua_State* L, event::Bus& bus) {
    sol::state_view lua(L);
    sol::table api = lua["api"].get_or_create<sol::table>();
    sol::table ev = api["event"].get_or_create<sol::table>();

    // Lua 注册的 handler 存这里 (sol2 类型仅在本 .cpp, shared_ptr 保持引用防 GC)。
    struct LuaHandler {
        std::shared_ptr<sol::protected_function> func;
        event::Bus::SubscriptionId id;
    };
    auto handlers = std::make_shared<std::unordered_map<std::string, std::vector<LuaHandler>>>();

    // api.event.on(name, func): 追加一个 Lua handler, 并向 bus 订阅桥接 lambda。
    ev["on"] = [handlers, &bus](std::string name, sol::protected_function func) {
        auto handler = std::make_shared<sol::protected_function>(func);
        const auto id = bus.on(name, [handler](const event::Value& payload) {
            // 载荷经 sol::state_view 转成 sol 对象传给 Lua 函数。
            sol::state_view state_view(handler->lua_state());
            const sol::protected_function_result result = (*handler)(to_sol_object(state_view, payload));
            if (!result.valid()) {
                const sol::error err = result;
                app_log::error(std::string("api.event handler error: ") + err.what());
            }
        });
        (*handlers)[name].push_back(LuaHandler{handler, id});
    };

    // api.event.off(name): 移除该事件名下所有 Lua 注册的 handler (含对应 bus 订阅)。
    ev["off"] = [handlers, &bus](std::string name) {
        const auto it = handlers->find(name);
        if (it == handlers->end()) {
            return;
        }
        for (const LuaHandler& entry : it->second) {
            bus.off(name, entry.id);
        }
        handlers->erase(it);
    };

    // api.event.emit(name, payload?): Lua 侧产生事件, 转 event::Value 后同步分发。
    ev["emit"] = [&bus](std::string name, sol::object payload) { bus.emit(name, to_event_value(payload)); };
}

void register_all(lua_engine::Engine& engine) {
    lua_State* L = engine.raw_state();
    register_log(L);
    register_util(L);
    // 后续: register_camera(L); register_ecs(L); ...
}
} // namespace api
