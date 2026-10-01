#include "camera/camera.h"
#include "script/api_registry.h"
#include "script/lua_engine.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>

namespace {
std::string lua_path(const char* name) {
    return std::string(PROJECT_PHOENIX_LUA_DIR) + "/" + name;
}

camera::Params isometric_params(float distance, float fovy = 45.0f) {
    camera::Params p;
    p.distance = distance;
    p.fovy = fovy;
    return p;
}
} // namespace

TEST_CASE("api.camera.* drives the camera manager from Lua") {
    lua_engine::Engine engine;
    camera::Manager manager;
    api::register_all(engine);
    api::register_camera(engine.raw_state(), manager);

    manager.create("main", camera::Type::Isometric, isometric_params(15.0f));
    manager.create("overview", camera::Type::Isometric, isometric_params(25.0f));
    manager.set_active("main");

    REQUIRE(engine.do_file(lua_path("test_camera.lua")));

    // switch 按名称切换。
    REQUIRE(engine.call_function("switch_to_overview").as_bool());
    REQUIRE(manager.active_name() == "overview");

    // get_active / get_active_id 读回名称与 id。
    REQUIRE(engine.call_function("current_name").as_string() == "overview");
    REQUIRE(engine.call_function("current_id").as_number() == 2.0);

    // switch 按 id 切回 main。
    REQUIRE(engine.call_function("switch_by_id").as_bool());
    REQUIRE(manager.active_name() == "main");

    // set_param 改 FOV / 距离, C++ 侧生效。
    REQUIRE(engine.call_function("change_fov").as_bool());
    REQUIRE(manager.active_params()->fovy == 60.0f);

    REQUIRE(engine.call_function("change_distance").as_bool());
    REQUIRE(manager.active_params()->distance == 20.0f);
    REQUIRE(manager.active_params()->pos[0] == Catch::Approx(camera::kIsoDirX * 20.0f));

    // list 返回名称表。
    REQUIRE(engine.call_function("camera_count").as_number() == 2.0);

    // 占位函数返回 false 不报错。
    REQUIRE_FALSE(engine.call_function("shake_placeholder").as_bool());
}

TEST_CASE("calling an unregistered api.camera fails gracefully") {
    lua_engine::Engine engine;
    api::register_all(engine);

    const std::string path = lua_path("_tmp_api_camera_unregistered.lua");
    {
        std::ofstream out(path);
        out << "api.camera.get_active()\n";
    }

    REQUIRE_FALSE(engine.do_file(path));

    REQUIRE(std::remove(path.c_str()) == 0);
}
