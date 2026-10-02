#pragma once

#include "app/game.h"
#include "camera/camera.h"
#include "ecs/ecs.h"
#include "event/event_bus.h"
#include "raylib.h"
#include "script/lua_engine.h"

namespace scene::main_game {
// P0-09 端到端集成 demo 场景: 验证「C++ 读配置 → ECS 建实体 → 事件分发 → Lua 处理 → 切相机」闭环。
class DemoScene : public app::Game {
  public:
    bool init() override;

    void update(float fixedDt) override;

    void render(float alpha) override;

    void renderUi() override;

    void shutdown() override;

  private:
    // demo 局部组件: 玩家出生位置。
    struct Position {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    lua_engine::Engine lua_;
    event::Bus bus_;
    camera::Manager cameraManager_;
    ecs::Registry registry_;

    ecs::Entity player_ = ecs::null;
    Position playerPos_{};

    Camera3D camera_{};
};
} // namespace scene::main_game
