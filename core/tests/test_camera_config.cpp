#include "camera/camera_config.h"
#include "config/config.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>

namespace {
std::string config_path(const char* name) {
    return std::string(PROJECT_PHOENIX_CONFIG_DIR) + "/" + name;
}
} // namespace

TEST_CASE("camera config parses entries, types, follow and active") {
    const auto doc = config::load_file(config_path("test_camera_config.json"));
    REQUIRE(doc.has_value());

    const std::optional<camera::Config> config = camera::load_config(*doc);
    REQUIRE(config.has_value());

    REQUIRE(config->cameras.size() == 2);
    REQUIRE(config->active == "main");

    // 条目 0: 透视, 显式 distance/fovy, 无跟随。
    const camera::CameraEntry& first = config->cameras[0];
    REQUIRE(first.name == "main");
    REQUIRE(first.params.type == camera::Type::Isometric);
    REQUIRE(first.params.distance == Catch::Approx(15.0));
    REQUIRE(first.params.fovy == Catch::Approx(45.0));
    REQUIRE_FALSE(first.follow_enabled);

    // 条目 1: 正交, 跟随平滑 0.25。
    const camera::CameraEntry& second = config->cameras[1];
    REQUIRE(second.name == "top_ortho");
    REQUIRE(second.params.type == camera::Type::IsometricOrtho);
    REQUIRE(second.params.distance == Catch::Approx(40.0));
    REQUIRE(second.params.fovy == Catch::Approx(25.0));
    REQUIRE(second.follow_enabled);
    REQUIRE(second.smoothing == Catch::Approx(0.25));
}

TEST_CASE("camera config rejects unknown type") {
    const auto doc = config::load_file(config_path("test_camera_config_invalid.json"));
    REQUIRE(doc.has_value());

    REQUIRE_FALSE(camera::load_config(*doc).has_value());
}
