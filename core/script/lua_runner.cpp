#include "script/lua_runner.h"

#include <lua.hpp>

namespace lua_runner {
    std::string eval_string(const std::string &script) {
        lua_State *L = luaL_newstate();
        luaL_openlibs(L);

        std::string result;
        if (luaL_dostring(L, script.c_str()) == LUA_OK) {
            const char *value = lua_tostring(L, -1);
            result = value ? value : "";
        } else {
            result = std::string("error: ") + lua_tostring(L, -1);
        }

        lua_close(L);
        return result;
    }
}
