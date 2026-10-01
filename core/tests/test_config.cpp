#include "config/config.h"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

namespace {
std::string config_path(const char* name) {
    return std::string(PROJECT_PHOENIX_CONFIG_DIR) + "/" + name;
}
} // namespace

TEST_CASE("load a valid JSON file and read basic fields") {
    const auto doc = config::load_file(config_path("test_basic.json"));
    REQUIRE(doc.has_value());

    REQUIRE(doc->get_string("title", "") == "demo");
    REQUIRE(doc->get_int("width", 0) == 800);
    REQUIRE(doc->get_int("height", 0) == 600);
    REQUIRE(doc->get_bool("fullscreen", true) == false);
    REQUIRE(doc->get_double("scale", 0.0) == 1.5);
}

TEST_CASE("read nested fields via dot-separated path") {
    const auto doc = config::load_file(config_path("test_basic.json"));
    REQUIRE(doc.has_value());

    REQUIRE(doc->get_string("window.title", "") == "nested");
    REQUIRE(doc->get_int("window.width", 0) == 1280);
}

TEST_CASE("default values returned for missing or mismatched fields") {
    const auto doc = config::load_file(config_path("test_basic.json"));
    REQUIRE(doc.has_value());

    REQUIRE(doc->get_int("missing", 42) == 42);
    REQUIRE(doc->get_string("missing", "fallback") == "fallback");
    REQUIRE(doc->get_int("title", 42) == 42);      // 类型不匹配 (string) 返回默认值
    REQUIRE(doc->get_bool("width", true) == true); // 类型不匹配 (number) 返回默认值
}

TEST_CASE("invalid JSON reports error and returns nullopt") {
    const auto doc = config::load_file(config_path("test_invalid.json"));
    REQUIRE_FALSE(doc.has_value());
}

TEST_CASE("validate required and optional fields") {
    const auto doc = config::load_file(config_path("test_basic.json"));
    REQUIRE(doc.has_value());

    // 全部顶层字段均已声明: ok() 为 true, 无 missing / unknown。
    const auto all = doc->validate({"title", "width", "height", "fullscreen", "scale", "window"});
    REQUIRE(all.ok());
    REQUIRE(all.missing.empty());
    REQUIRE(all.unknown.empty());

    // 部分声明: 未声明的 fullscreen/scale/window 进入 unknown, 无缺失必填。
    const auto partial = doc->validate({"title", "width"}, {"height"});
    REQUIRE_FALSE(partial.ok()); // 因存在 unknown
    REQUIRE(partial.missing.empty());
    REQUIRE(partial.unknown.size() == 3);
    for (const char* key : {"fullscreen", "scale", "window"}) {
        REQUIRE(std::find(partial.unknown.begin(), partial.unknown.end(), key) != partial.unknown.end());
    }

    // 缺失必填字段: nonexistent 进入 missing。
    const auto missing = doc->validate({"title", "nonexistent"});
    REQUIRE_FALSE(missing.ok());
    REQUIRE(missing.missing == std::vector<std::string>{"nonexistent"});
}
