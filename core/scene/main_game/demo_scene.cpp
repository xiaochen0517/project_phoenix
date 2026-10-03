#include "scene/main_game/demo_scene.h"

#include "camera/camera_raylib.h"
#include "config/config.h"
#include "imgui.h"
#include "log/app_log.h"
#include "script/api_registry.h"
#include "util/asset_path.h"

#include <cstdint>
#include <string>

namespace {
constexpr Color kBackgroundColor{24, 28, 32, 255};
} // namespace

namespace scene::main_game {
bool DemoScene::init() {
    // 1. C++ 加载并解析配置 (配置仅供 C++ 核心使用, 不经 Lua)。
    const auto doc = config::load_file(asset_path::resolve("config/demo_scene.json"));
    double defaultDistance = 18.0;
    double defaultFov = 45.0;
    double overviewDistance = 32.0;
    double overviewFov = 35.0;
    if (doc) {
        defaultDistance = doc->get_double("camera.default.distance", defaultDistance);
        defaultFov = doc->get_double("camera.default.fov", defaultFov);
        overviewDistance = doc->get_double("camera.overview.distance", overviewDistance);
        overviewFov = doc->get_double("camera.overview.fov", overviewFov);
        playerPos_.x = static_cast<float>(doc->get_double("player.x", 0.0));
        playerPos_.y = static_cast<float>(doc->get_double("player.y", 0.0));
        playerPos_.z = static_cast<float>(doc->get_double("player.z", 0.0));
    } else {
        app_log::warn("demo_scene: config load failed, use defaults");
    }

    // 2. 创建两个 isometric 相机 (参数来自配置)。
    camera::Params params;
    params.distance = static_cast<float>(defaultDistance);
    params.fovy = static_cast<float>(defaultFov);
    cameraManager_.create("default", params);
    params.distance = static_cast<float>(overviewDistance);
    params.fovy = static_cast<float>(overviewFov);
    cameraManager_.create("overview", params);
    cameraManager_.set_active("default");

    // 3. 注册 api.* 绑定 (无状态 + event + camera)。
    api::register_all(lua_);
    api::register_event(lua_.raw_state(), bus_);
    api::register_camera(lua_.raw_state(), cameraManager_);

    // 4. Lua 订阅事件。
    lua_.do_file(asset_path::resolve("lua/demo_scene.lua"));

    // 5. ECS (C++) 创建玩家实体。
    player_ = registry_.create();
    registry_.attach<Position>(player_, playerPos_);

    // 6. 触发事件 → 同步分发到 Lua handler → Lua 处理并 api.camera.switch。
    bus_.emit("player_spawn", event::Value::number(static_cast<double>(static_cast<std::uint32_t>(player_))));

    // 7. 渲染相机同步为活动相机。
    camera_ = camera::to_camera3d(*cameraManager_.active_params());

    return true;
}

void DemoScene::update(float fixedDt) {
    (void)fixedDt;
}

void DemoScene::render(float alpha) {
    (void)alpha;
    ClearBackground(kBackgroundColor);
    BeginMode3D(camera_);
    DrawGrid(20, 1.0f);
    DrawCube({playerPos_.x, playerPos_.y + 0.5f, playerPos_.z}, 1.0f, 1.0f, 1.0f, RED);
    EndMode3D();
}

void DemoScene::renderUi() {
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360, 0), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Demo (端到端集成)")) {
        ImGui::Text("FPS: %d", GetFPS());
        ImGui::Separator();
        ImGui::Text("玩家实体: id=%u", static_cast<std::uint32_t>(player_));
        ImGui::Text("出生点: (%.1f, %.1f, %.1f)", playerPos_.x, playerPos_.y, playerPos_.z);
        ImGui::Text("活动相机: %s", cameraManager_.active_name().c_str());
    }
    ImGui::End();
}

void DemoScene::shutdown() {
}
} // namespace scene::main_game
