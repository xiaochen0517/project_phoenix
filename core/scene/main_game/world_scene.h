#pragma once

#include "app/game.h"
#include "camera/camera.h"
#include "ecs/ecs.h"
#include "input/input.h"
#include "model/model_loader.h"
#include "raylib.h"
#include "visibility/visibility.h"
#include "world/world.h"

#include <cstdint>
#include <optional>

namespace scene::main_game {
// 世界场景: 稀疏房屋地图 (x,y 水平 + z 层级) + bob 玩家模型, 串起 input → movement → 实体位置。
// 渲染: 地面 2D / 遮挡判定 (visibility: 高层溶解 + 墙裁剪) / 垂直缩放 3m 墙高 / 网格描边 / 自由相机 / 相机调试。
class WorldScene : public app::Game {
  public:
    bool init() override;
    void update(float fixedDt) override;
    void render(float alpha) override;
    void renderUi() override;
    void shutdown() override;

  private:
    void renderLevel(std::int32_t z);
    void renderWalls(const world::Level& level, float baseY);
    void renderPlayer();
    void syncCameraToPlayer();

    std::optional<world::World> world_;
    input::Manager input_;
    camera::Manager camera_;
    ecs::Registry registry_;
    model_loader::ModelScene playerModel_;
    float feetOffset_[3] = {0.0f, 0.0f, 0.0f};

    ecs::Entity player_ = ecs::null;
    visibility::Culler culler_;
    Camera3D camera3d_{};

    bool showGrid_ = true;
    bool freeCamera_ = false;
    float cameraMoveSpeed_ = 10.0f;
    float cameraRotateSpeed_ = 0.003f;
    bool freeCapturing_ = false;
};
} // namespace scene::main_game
