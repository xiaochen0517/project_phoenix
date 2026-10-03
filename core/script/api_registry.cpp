#include "script/api_registry.h"

#include "camera/camera.h"
#include "event/event_bus.h"
#include "input/input.h"
#include "log/app_log.h"
#include "script/lua_engine.h"

#include <cstddef>
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

// 活动上下文绑定表指针; 无活动上下文返回 nullptr。
input::BindingMap* active_bindings(input::Manager& manager) {
    if (!manager.has_context(manager.active_context())) {
        return nullptr;
    }
    return &manager.context(manager.active_context());
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
    // 后续: register_camera(L, manager); register_ecs(L); ...
    // 注意: register_camera / register_event 依赖有状态对象 (camera::Manager / event::Bus),
    //       与 register_all 分开, 由调用方注入实例 (见 tests 与 app 层)。
}

void register_camera(lua_State* L, camera::Manager& manager) {
    sol::state_view lua(L);
    sol::table api = lua["api"].get_or_create<sol::table>();
    sol::table cam = api["camera"].get_or_create<sol::table>();

    // 活动相机: 名称(string) + id(number) 双标识。
    cam["get_active"] = [&manager]() { return manager.active_name(); };
    cam["get_active_id"] = [&manager]() { return static_cast<double>(manager.active()); };

    // switch(name_or_id): 名称或 id 切换活动相机, 返回是否成功。
    cam["switch"] = sol::overload([&manager](const std::string& name) { return manager.set_active(name); },
                                  [&manager](double id) { return manager.set_active(static_cast<camera::Id>(id)); });

    // set_param(key, value): 改活动相机参数 (value 为 number), 返回是否成功。
    cam["set_param"] = [&manager](const std::string& key, double value) {
        return manager.set_param_active(key, value);
    };

    // list(): 返回相机名称表 (string 数组, 1-based)。
    cam["list"] = [&manager, L]() {
        sol::state_view lua(L);
        sol::table table = lua.create_table();
        const std::vector<std::string> names = manager.names();
        for (std::size_t i = 0; i < names.size(); ++i) {
            table[i + 1] = names[i];
        }
        return table;
    };

    // set_follow(entity, smoothing): P1-04 实装 —— 活动相机跟随目标实体。
    // entity 为 uint32 语义 number（对齐 api.ecs 实体 id，P1-05 落地后可从 api.ecs 获取;
    // 0 是合法实体，清除跟随用 clear_follow）; smoothing > 0（指数平滑速率）。
    cam["set_follow"] = [&manager](double entity, double smoothing) {
        return manager.set_follow_active(static_cast<std::uint32_t>(entity), static_cast<float>(smoothing));
    };

    // clear_follow(): 清除活动相机跟随。
    cam["clear_follow"] = [&manager]() { return manager.clear_follow_active(); };

    // get_type(): 活动相机类型字符串; 无活动相机返回 nil。
    cam["get_type"] = [&manager, L]() -> sol::object {
        const camera::Params* params = manager.active_params();
        if (params == nullptr) {
            return sol::nil;
        }
        sol::state_view state(L);
        return sol::make_object(state, std::string(camera::type_name(params->type)));
    };

    // set_type(type_string): 切换活动相机类型（"isometric" / "isometric_ortho"），返回是否成功。
    cam["set_type"] = [&manager](const std::string& type_name) {
        const std::optional<camera::Type> type = camera::type_from_name(type_name);
        return type.has_value() && manager.set_type_active(*type);
    };

    // 占位（后续需求补齐）: 固定返回 false, 保证调用不报错。
    cam["shake"] = [](sol::variadic_args) { return false; };
}

void register_input(lua_State* L, input::Manager& manager) {
    sol::state_view lua(L);
    sol::table api = lua["api"].get_or_create<sol::table>();
    sol::table in = api["input"].get_or_create<sol::table>();

    // bind(action, key, force?): 在活动上下文绑定, 冲突且未 force 返回 false。
    in["bind"] = sol::overload(
        [&manager](std::string action, std::string key_name) -> bool {
            input::BindingMap* bindings = active_bindings(manager);
            const input::Key key = input::key_from_name(key_name);
            return bindings != nullptr && key != input::Key::Unknown &&
                   bindings->bind(action, key) == input::BindResult::Ok;
        },
        [&manager](std::string action, std::string key_name, bool force) -> bool {
            input::BindingMap* bindings = active_bindings(manager);
            const input::Key key = input::key_from_name(key_name);
            return bindings != nullptr && key != input::Key::Unknown &&
                   bindings->bind(action, key, force) == input::BindResult::Ok;
        });

    // unbind(action): 解绑活动上下文中的动作。
    in["unbind"] = [&manager](std::string action) -> bool {
        input::BindingMap* bindings = active_bindings(manager);
        return bindings != nullptr && bindings->unbind(action);
    };

    // query(action): 返回动作绑定的键名 (string), 未绑定返回 nil。
    in["query"] = [&manager, L](std::string action) -> sol::object {
        const std::optional<input::Key> key = manager.query(action);
        if (!key.has_value()) {
            return sol::nil;
        }
        sol::state_view state(L);
        return sol::make_object(state, input::key_name(*key));
    };

    // actions(key): 返回该键绑定的动作名表 (string 数组, 1-based)。
    in["actions"] = [&manager, L](std::string key_name) {
        sol::state_view state(L);
        sol::table table = state.create_table();
        const input::Key key = input::key_from_name(key_name);
        if (key == input::Key::Unknown) {
            return table;
        }
        input::BindingMap* bindings = active_bindings(manager);
        if (bindings == nullptr) {
            return table;
        }
        const std::vector<std::string> actions = bindings->actions_for(key);
        for (std::size_t i = 0; i < actions.size(); ++i) {
            table[i + 1] = actions[i];
        }
        return table;
    };

    // state(action): 返回动作当前状态字符串 ("idle"/"pressed"/"held"/"released")。
    in["state"] = [&manager](std::string action) -> std::string {
        switch (manager.state(action)) {
        case input::ActionState::Pressed:
            return "pressed";
        case input::ActionState::Held:
            return "held";
        case input::ActionState::Released:
            return "released";
        case input::ActionState::Idle:
        default:
            return "idle";
        }
    };

    // key(name): 键名 → key code (辅助), 未知返回 nil。
    in["key"] = [L](std::string name) -> sol::object {
        const input::Key key = input::key_from_name(name);
        if (key == input::Key::Unknown) {
            return sol::nil;
        }
        sol::state_view state(L);
        return sol::make_object(state, static_cast<double>(key));
    };
}
} // namespace api
