#include "event/event_bus.h"
#include "script/api_registry.h"
#include "script/lua_engine.h"

#include <catch2/catch_test_macros.hpp>
#include <string>

namespace {
std::string lua_path(const char* name) {
    return std::string(PROJECT_PHOENIX_LUA_DIR) + "/" + name;
}
} // namespace

TEST_CASE("C++ emit triggers a Lua-registered handler") {
    lua_engine::Engine engine;
    event::Bus bus;
    api::register_all(engine);
    api::register_event(engine.raw_state(), bus);

    REQUIRE(engine.do_file(lua_path("test_event_cpp_to_lua.lua")));

    bus.emit("evt", event::Value::number(42.0));

    const lua_engine::Value value = engine.call_function("was_called");
    REQUIRE(value.is_bool());
    REQUIRE(value.as_bool());
}

TEST_CASE("C++ emit triggers all Lua handlers for the same event") {
    lua_engine::Engine engine;
    event::Bus bus;
    api::register_all(engine);
    api::register_event(engine.raw_state(), bus);

    REQUIRE(engine.do_file(lua_path("test_event_cpp_to_lua.lua")));

    bus.emit("evt_multi");

    const lua_engine::Value value = engine.call_function("handler_count");
    REQUIRE(value.is_number());
    REQUIRE(value.as_number() == 2.0);
}

TEST_CASE("Lua emit reaches a C++ subscriber") {
    lua_engine::Engine engine;
    event::Bus bus;
    api::register_all(engine);
    api::register_event(engine.raw_state(), bus);

    bool received = false;
    double received_value = 0.0;
    bus.on("lua_evt", [&received, &received_value](const event::Value& payload) {
        received = true;
        received_value = payload.as_number();
    });

    REQUIRE(engine.do_file(lua_path("test_event_lua_to_cpp.lua")));

    REQUIRE(received);
    REQUIRE(received_value == 123.0);
}

TEST_CASE("api.event.off removes the Lua handler") {
    lua_engine::Engine engine;
    event::Bus bus;
    api::register_all(engine);
    api::register_event(engine.raw_state(), bus);

    // 脚本在加载时已注册并 off 掉 "evt_off", C++ 再 emit 不应触发。
    REQUIRE(engine.do_file(lua_path("test_event_cpp_to_lua.lua")));

    bus.emit("evt_off");

    const lua_engine::Value value = engine.call_function("off_was_called");
    REQUIRE(value.is_bool());
    REQUIRE_FALSE(value.as_bool());
}

TEST_CASE("pure C++ bus dispatches synchronously to all subscribers") {
    event::Bus bus;
    int calls = 0;
    bus.on("evt", [&calls](const event::Value&) { ++calls; });
    bus.on("evt", [&calls](const event::Value&) { ++calls; });

    bus.emit("evt");

    REQUIRE(calls == 2);
}
