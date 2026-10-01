#include "script/lua_runner.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("eval_string evaluates a simple expression") {
    REQUIRE(lua_runner::eval_string("return 1 + 2") == "3");
}

TEST_CASE("eval_string returns a string result") {
    REQUIRE(lua_runner::eval_string("return 'hello'") == "hello");
}

TEST_CASE("eval_string executes a multi-line script") {
    const std::string script =
        "local s = 0\n"
        "for i = 1, 10 do s = s + i end\n"
        "return s";
    REQUIRE(lua_runner::eval_string(script) == "55");
}

TEST_CASE("eval_string returns empty string for nil result") {
    REQUIRE(lua_runner::eval_string("return nil") == "");
}

TEST_CASE("eval_string returns error prefix on failure") {
    const std::string result = lua_runner::eval_string("this is not valid lua");
    REQUIRE(result.rfind("error: ", 0) == 0);
}
