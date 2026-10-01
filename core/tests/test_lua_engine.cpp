#include "script/lua_engine.h"

#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <fstream>
#include <string>

namespace {
std::string lua_path(const char* name) {
    return std::string(PROJECT_PHOENIX_LUA_DIR) + "/" + name;
}
} // namespace

TEST_CASE("loads a Lua file and calls a function returning a number") {
    lua_engine::Engine engine;
    REQUIRE(engine.do_file(lua_path("test_engine_basic.lua")));

    const lua_engine::Value value = engine.call_function("get_value");
    REQUIRE(value.is_number());
    REQUIRE(value.as_number() == 42.0);
}

TEST_CASE("loads a Lua file and calls a function returning a string") {
    lua_engine::Engine engine;
    REQUIRE(engine.do_file(lua_path("test_engine_basic.lua")));

    const lua_engine::Value value = engine.call_function("get_text");
    REQUIRE(value.is_string());
    REQUIRE(value.as_string() == "hello");
}

TEST_CASE("reload picks up the new return value") {
    const std::string path = lua_path("_tmp_reload.lua");
    {
        std::ofstream out(path);
        out << "function get_value() return 1 end\n";
    }

    lua_engine::Engine engine;
    REQUIRE(engine.do_file(path));
    REQUIRE(engine.call_function("get_value").as_number() == 1.0);

    {
        std::ofstream out(path);
        out << "function get_value() return 2 end\n";
    }
    REQUIRE(engine.reload(path));
    REQUIRE(engine.call_function("get_value").as_number() == 2.0);

    REQUIRE(std::remove(path.c_str()) == 0);
}

TEST_CASE("do_file reports failure for a missing file") {
    lua_engine::Engine engine;
    REQUIRE_FALSE(engine.do_file(lua_path("__does_not_exist__.lua")));
}
