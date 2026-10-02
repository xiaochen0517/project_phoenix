#pragma once

namespace app {
// 业务载体抽象接口: 由框架外壳 App 驱动生命周期。
// 具体业务场景 (模型查看器、demo 场景、后续真实游戏场景) 实现本接口。
class Game {
  public:
    virtual ~Game() = default;

    // 业务初始化 (窗口 / ImGui 已就绪后调用); 返回 false 表示初始化失败。
    virtual bool init() = 0;

    // 固定 30Hz 逻辑更新, fixedDt 为固定步长。
    virtual void update(float fixedDt) = 0;

    // 每帧 3D 渲染 (ClearBackground + BeginMode3D/EndMode3D); alpha 为当前帧在两次固定 tick 之间的插值系数 (∈ [0,1))。
    // 渲染帧所有权 (Begin/EndDrawing + rlImGuiBegin/End) 由持有帧的一方 (如 SceneHost) 负责。
    virtual void render(float alpha) = 0;

    // 本场景 ImGui 面板, 由持有帧的一方在 rlImGuiBegin/End 之间调用。
    virtual void renderUi() = 0;

    // 业务卸载 (窗口关闭前调用)。
    virtual void shutdown() = 0;
};
} // namespace app
