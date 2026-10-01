#include "script/api_registry.h"

#include <sol/sol.hpp>

#include "log/app_log.h"
#include "script/lua_engine.h"

namespace api {
    namespace {
        // 绑定 api.log.*: 直接把 app_log 的日志函数暴露给 Lua。
        void register_log(lua_State *L) {
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
        void register_util(lua_State *L) {
            sol::state_view lua(L);
            sol::table api = lua["api"].get_or_create<sol::table>();
            sol::table util = api["util"].get_or_create<sol::table>();
            util["add"] = [](double a, double b) { return a + b; };
        }
    } // namespace

    void register_all(lua_engine::Engine &engine) {
        lua_State *L = engine.raw_state();
        register_log(L);
        register_util(L);
        // 后续: register_event(L); register_camera(L); register_ecs(L); ...
    }
} // namespace api
