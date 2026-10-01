#include "script/api_registry.h"
#include "script/lua_engine.h"

#include <catch2/catch_test_macros.hpp>

#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

namespace {
    std::string lua_path(const char *name) {
        return std::string(PROJECT_PHOENIX_LUA_DIR) + "/" + name;
    }
} // namespace

TEST_CASE("api.log.info is callable from Lua and outputs the message") {
    lua_engine::Engine engine;
    api::register_all(engine);
    REQUIRE(engine.do_file(lua_path("test_api_log.lua")));

    // 捕获 spdlog 输出: 用临时 ostream sink 替换默认 logger, 断言消息出现后恢复。
    std::ostringstream stream;
    const auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(stream);
    const auto logger = std::make_shared<spdlog::logger>("capture", sink);
    const auto previous = spdlog::default_logger();
    spdlog::set_default_logger(logger);

    engine.call_function("emit_log");

    spdlog::set_default_logger(previous);

    REQUIRE(stream.str().find("from_lua") != std::string::npos);
}

TEST_CASE("api.util.add returns a value to Lua") {
    lua_engine::Engine engine;
    api::register_all(engine);
    REQUIRE(engine.do_file(lua_path("test_api_value.lua")));

    const lua_engine::Value value = engine.call_function("api_value");
    REQUIRE(value.is_number());
    REQUIRE(value.as_number() == 5.0);
}

TEST_CASE("calling an unregistered api module fails gracefully") {
    lua_engine::Engine engine;
    api::register_all(engine);

    const std::string path = lua_path("_tmp_api_unregistered.lua");
    {
        std::ofstream out(path);
        out << "api.event.on('x', function() end)\n";
    }

    REQUIRE_FALSE(engine.do_file(path));

    REQUIRE(std::remove(path.c_str()) == 0);
}
