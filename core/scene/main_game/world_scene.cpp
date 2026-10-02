#include "scene/main_game/world_scene.h"

#include "camera/camera_raylib.h"
#include "camera/free_fly.h"
#include "config/config.h"
#include "imgui.h"
#include "input/input_raylib.h"
#include "log/app_log.h"
#include "movement/movement.h"
#include "rlgl.h"
#include "util/asset_path.h"

#include <cstdint>

namespace {
constexpr Color kBackgroundColor{24, 28, 32, 255};
constexpr Color kGridColor{28, 28, 28, 200};
constexpr Color kWallColor{76, 57, 38, 255};        // 深棕色 (正常)
constexpr Color kWallColorOccluded{76, 57, 38, 80}; // 深棕色半透明 (被遮挡)
constexpr float kPlayerSpawnX = 10.5f;              // 房屋南侧草地 (房屋 x/y ∈ [8,12], 南墙门洞 x=10, 出生点正对门洞)
constexpr float kPlayerSpawnY = 15.5f;

Color to_color(const world::TileDef& def) {
    return Color{static_cast<unsigned char>(def.r * 255.0f), static_cast<unsigned char>(def.g * 255.0f),
                 static_cast<unsigned char>(def.b * 255.0f), 255};
}

// 竖直墙面平面: 2D 四边形 (双面几何), 位于地面 tile 的接缝处。
// 注意: raylib 默认启用背面剔除, 且为延迟批量渲染 —— rlVertex3f 仅把顶点累积进批处理缓冲,
// 真正的 glDraw 在批次刷新 (EndDrawing / 纹理切换) 时才执行, 届时剔除状态已恢复,
// 故绘制期间临时关闭剔除无效。此处采用双面几何: 正反两个绕序相反的四边面,
// 剔除开启时任意视角恰好有一面通过 (不会 z-fight, 半透明墙也只混合一次)。
// rotAxis: 1 = 绕 X 轴立起 (南北墙, 接缝沿世界 X), 2 = 绕 Z 轴立起 (东西墙, 接缝沿世界 Z)。
// seamLen = 墙沿接缝方向的长度 (1 格); height = 墙高 (level_height × vertical_scale)。
void draw_wall_plane(Vector3 center, float seamLen, float height, Color color, int rotAxis) {
    rlPushMatrix();
    rlTranslatef(center.x, center.y, center.z);
    if (rotAxis == 1) {
        rlRotatef(90.0f, 1.0f, 0.0f, 0.0f); // 绕 X 轴立起: 局部 Z → 世界垂直, 局部 X 保持沿接缝
    } else {
        rlRotatef(90.0f, 0.0f, 0.0f, 1.0f); // 绕 Z 轴立起: 局部 X → 世界垂直, 局部 Z 保持沿接缝
    }

    // 局部坐标 (旋转前的 XZ 平面): 接缝方向与垂直方向随旋转轴互换。
    const float ax = (rotAxis == 1) ? seamLen : height; // 局部 X 半长
    const float az = (rotAxis == 1) ? height : seamLen; // 局部 Z 半长
    const Vector3 v[4] = {
        {-ax * 0.5f, 0.0f, -az * 0.5f},
        {-ax * 0.5f, 0.0f, az * 0.5f},
        {ax * 0.5f, 0.0f, az * 0.5f},
        {ax * 0.5f, 0.0f, -az * 0.5f},
    };

    rlBegin(RL_QUADS);
    rlColor4ub(color.r, color.g, color.b, color.a);
    rlNormal3f(0.0f, 1.0f, 0.0f);
    // 正面。
    rlVertex3f(v[0].x, v[0].y, v[0].z);
    rlVertex3f(v[1].x, v[1].y, v[1].z);
    rlVertex3f(v[2].x, v[2].y, v[2].z);
    rlVertex3f(v[3].x, v[3].y, v[3].z);
    // 反面 (绕序相反)。
    rlVertex3f(v[3].x, v[3].y, v[3].z);
    rlVertex3f(v[2].x, v[2].y, v[2].z);
    rlVertex3f(v[1].x, v[1].y, v[1].z);
    rlVertex3f(v[0].x, v[0].y, v[0].z);
    rlEnd();
    rlPopMatrix();
}
} // namespace

namespace scene::main_game {
bool WorldScene::init() {
    // 1. 加载世界地图 (config/demo_world.json)。
    const auto doc = config::load_file(asset_path::resolve("config/demo_world.json"));
    if (!doc) {
        app_log::error("world_scene: config/demo_world.json load failed");
        return false;
    }
    auto world = world::World::load(*doc);
    if (!world) {
        app_log::error("world_scene: world load failed");
        return false;
    }
    world_ = std::move(world);

    // 2. 加载输入绑定; 缺失时兜底内置 W/A/S/D。
    const auto bindings = config::load_file(asset_path::resolve("config/input_bindings.json"));
    if (bindings) {
        input::load_bindings(input_, *bindings);
    } else {
        app_log::warn("world_scene: input_bindings.json load failed, using built-in defaults");
    }
    if (!input_.has_context("main_game")) {
        input::BindingMap& ctx = input_.context("main_game");
        ctx.bind("move_up", input::Key::W);
        ctx.bind("move_down", input::Key::S);
        ctx.bind("move_left", input::Key::A);
        ctx.bind("move_right", input::Key::D);
    }
    input_.set_context("main_game");

    // 3. 创建玩家实体 (Position + Mover), 出生在房屋南侧草地 (正对南墙门洞)。
    player_ = registry_.create();
    registry_.attach<ecs::Position>(player_, ecs::Position{kPlayerSpawnX, kPlayerSpawnY, 0.0f});
    registry_.attach<ecs::Mover>(player_, ecs::Mover{3.0f, 0});

    // 4. 加载玩家模型 (bob_idle), 播放 idle 动画。
    const std::string modelPath = asset_path::resolve("assets/models/body/Bob_Idle.glb");
    std::string error;
    if (!playerModel_.load(modelPath, &error)) {
        app_log::error("world_scene: player model load failed: " + error);
        return false;
    }
    float center[3];
    float size[3];
    if (playerModel_.bounds(center, size)) {
        feetOffset_[0] = -center[0];
        feetOffset_[1] = -(center[1] - size[1] * 0.5f);
        feetOffset_[2] = -center[2];
    }
    if (playerModel_.animationCount() > 0) {
        playerModel_.setActiveAnimation(0);
        playerModel_.setLoop(true);
        playerModel_.setPlaying(true);
    }

    // 5. isometric 相机, 目标跟随玩家。
    camera::Params params;
    params.distance = 30.0f;
    params.target[0] = kPlayerSpawnX;
    params.target[1] = 0.0f;
    params.target[2] = kPlayerSpawnY;
    camera_.create("world", camera::Type::Isometric, params);
    camera_.set_active("world");
    syncCameraToPlayer();

    return true;
}

void WorldScene::update(float fixedDt) {
    if (!world_ || player_ == ecs::null) {
        return;
    }

    // 自由相机模式下不推进玩家移动 (W/A/S/D 由自由相机占用)。
    if (freeCamera_) {
        return;
    }

    // 1. 采样输入。
    input_.sample(input::sample_pressed_keys());

    // 2. 合成移动方向 (叠加所有 Held 动作的方向向量)。
    float dx = 0.0f;
    float dy = 0.0f;
    const char* actions[] = {"move_up", "move_down", "move_left", "move_right"};
    for (const char* action : actions) {
        const input::ActionState state = input_.state(action);
        if (state == input::ActionState::Held || state == input::ActionState::Pressed) {
            if (const auto dir = input::action_to_dir(action)) {
                dx += dir->x;
                dy += dir->y;
            }
        }
    }

    // 3. 移动 + 碰撞 (纯逻辑)。
    ecs::Position& pos = registry_.get<ecs::Position>(player_);
    ecs::Mover& mover = registry_.get<ecs::Mover>(player_);
    movement::step(*world_, pos, mover, dx, dy, fixedDt);

    // 4. 更新遮挡判定 (高层溶解 + 同层墙透明)。
    culler_.update(*world_, visibility::View{pos.x, pos.y, mover.z});

    // 5. 相机目标跟随玩家。
    syncCameraToPlayer();
}

void WorldScene::render(float alpha) {
    (void)alpha;
    playerModel_.update(GetFrameTime());

    if (freeCamera_) {
        camera::update_free_fly(camera3d_, cameraMoveSpeed_, cameraRotateSpeed_, freeCapturing_);
    }

    ClearBackground(kBackgroundColor);
    BeginMode3D(camera3d_);

    if (world_ && player_ != ecs::null) {
        const ecs::Mover& mover = registry_.get<ecs::Mover>(player_);

        // 高层溶解: culler 判定 (屋内 / 建筑在玩家东南前侧) 时跳过所有更高层 (z > 玩家层)。
        for (const std::int32_t z : world_->level_zs()) {
            if (culler_.upper_dissolved() && z > mover.z) {
                continue;
            }
            renderLevel(z);
        }
        renderPlayer();
    }

    EndMode3D();
}

void WorldScene::renderLevel(std::int32_t z) {
    if (!world_) {
        return;
    }
    const world::Level* lvl = world_->level(z);
    if (lvl == nullptr) {
        return;
    }
    const float baseY = static_cast<float>(z) * world_->level_height() * world_->vertical_scale();

    for (const auto& entry : lvl->tiles) {
        std::int32_t x = 0;
        std::int32_t y = 0;
        world::tile_key_decode(entry.first, x, y);
        const world::TileDef* def = world_->tile_def(entry.second);
        if (def == nullptr) {
            continue;
        }

        // 地图 (x, y) 水平 + z 层级 → raylib (x, z, y) (raylib y = up)。
        const float rx = static_cast<float>(x) + 0.5f;
        const float rz = static_cast<float>(y) + 0.5f;
        const float h = def->height * world_->vertical_scale();
        const Color color = to_color(*def);

        if (h <= 0.001f) {
            DrawPlane({rx, baseY, rz}, {1.0f, 1.0f}, color);
        } else {
            DrawCube({rx, baseY + h * 0.5f, rz}, 1.0f, h, 1.0f, color);
        }

        if (showGrid_) {
            if (h <= 0.001f) {
                DrawCubeWires({rx, baseY, rz}, 1.0f, 0.001f, 1.0f, kGridColor);
            } else {
                DrawCubeWires({rx, baseY + h * 0.5f, rz}, 1.001f, h, 1.001f, kGridColor);
            }
        }
    }

    renderWalls(*lvl, baseY);
}

void WorldScene::renderWalls(const world::Level& level, float baseY) {
    const float wallHeight = world_->level_height() * world_->vertical_scale();
    const float yCenter = baseY + wallHeight * 0.5f;
    for (const world::Wall& wall : level.walls) {
        const float wx = static_cast<float>(wall.cell.x);
        const float wy = static_cast<float>(wall.cell.y);

        const bool hidden = culler_.is_wall_hidden(wall);
        const Color color = hidden ? kWallColorOccluded : kWallColor;

        // 墙面 = 2D 平面, 渲染在地面 tile 的接缝处 (墙线)。
        switch (wall.side) {
        case world::Side::North:
            // 北墙: 接缝 z = wy, 沿 x ∈ [wx, wx+1]。
            draw_wall_plane({wx + 0.5f, yCenter, wy}, 1.0f, wallHeight, color, 1);
            break;
        case world::Side::South:
            // 南墙: 接缝 z = wy + 1。
            draw_wall_plane({wx + 0.5f, yCenter, wy + 1.0f}, 1.0f, wallHeight, color, 1);
            break;
        case world::Side::East:
            // 东墙: 接缝 x = wx + 1, 沿 z ∈ [wy, wy+1]。
            draw_wall_plane({wx + 1.0f, yCenter, wy + 0.5f}, 1.0f, wallHeight, color, 2);
            break;
        case world::Side::West:
            // 西墙: 接缝 x = wx。
            draw_wall_plane({wx, yCenter, wy + 0.5f}, 1.0f, wallHeight, color, 2);
            break;
        }
    }
}

void WorldScene::renderPlayer() {
    if (!playerModel_.isLoaded() || player_ == ecs::null) {
        return;
    }
    const ecs::Position& pos = registry_.get<ecs::Position>(player_);
    model_loader::Transform transform;
    // 地图 (x, y, z) → raylib (x, z, y)。
    transform.position[0] = pos.x + feetOffset_[0];
    transform.position[1] = pos.z * world_->vertical_scale() + feetOffset_[1];
    transform.position[2] = pos.y + feetOffset_[2];
    playerModel_.draw(transform, false);
}

void WorldScene::syncCameraToPlayer() {
    if (!world_ || player_ == ecs::null) {
        return;
    }
    const ecs::Position& pos = registry_.get<ecs::Position>(player_);
    // 地图 (x, y, z) → raylib (x, z, y); 高度经垂直缩放 (与 renderPlayer 一致)。
    camera_.set_param_active("target_x", pos.x);
    camera_.set_param_active("target_y", pos.z * world_->vertical_scale());
    camera_.set_param_active("target_z", pos.y);
    camera3d_ = camera::to_camera3d(*camera_.active_params());
}

void WorldScene::renderUi() {
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380, 0), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("World (渲染与移动)")) {
        ImGui::Text("FPS: %d", GetFPS());
        ImGui::Separator();
        if (player_ != ecs::null) {
            const ecs::Position& pos = registry_.get<ecs::Position>(player_);
            const ecs::Mover& mover = registry_.get<ecs::Mover>(player_);
            ImGui::Text("玩家位置: (%.2f, %.2f, z=%.2f)", pos.x, pos.y, pos.z);
            ImGui::Text("当前层: %d", static_cast<int>(mover.z));
        }
        ImGui::Text("操作: W/A/S/D 移动");
        ImGui::Separator();

        ImGui::Checkbox("显示网格", &showGrid_);

        ImGui::Text("遮挡: 高层溶解=%s", culler_.upper_dissolved() ? "是" : "否");

        if (ImGui::Checkbox("自由相机", &freeCamera_)) {
            if (!freeCamera_) {
                if (freeCapturing_) {
                    freeCapturing_ = false;
                    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
                    EnableCursor();
                }
                syncCameraToPlayer();
            }
        }

        if (ImGui::CollapsingHeader("相机", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (freeCamera_) {
                ImGui::SliderFloat("移动速度", &cameraMoveSpeed_, 0.1f, 50.0f);
                ImGui::SliderFloat("旋转速度", &cameraRotateSpeed_, 0.0005f, 0.01f, "%.4f");
                ImGui::TextDisabled("W/A/S/D 移动, Space/Ctrl 升降, 右键旋转");
            } else {
                bool changed = false;
                const camera::Params* params = camera_.active_params();
                if (params != nullptr) {
                    float distance = params->distance;
                    if (ImGui::DragFloat("距离", &distance, 0.1f)) {
                        changed |= camera_.set_param_active("distance", distance);
                    }
                    float fovy = params->fovy;
                    if (ImGui::SliderFloat("FOV", &fovy, 20.0f, 120.0f)) {
                        changed |= camera_.set_param_active("fov", fovy);
                    }
                    if (changed) {
                        syncCameraToPlayer();
                    }
                }
            }
        }
    }
    ImGui::End();
}

void WorldScene::shutdown() {
    playerModel_.unload();
}
} // namespace scene::main_game
