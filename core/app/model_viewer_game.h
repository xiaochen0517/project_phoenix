#pragma once

#include "app/game.h"
#include "model/model_loader.h"
#include "raylib.h"

namespace app {
// P0 阶段的运行载体: 迁移自原 main.cpp 的模型查看器 (模型 / 相机 / ImGui 面板)。
// 作为固定 tick 框架的验证载体, 不新增业务逻辑。
class ModelViewerGame : public Game {
  public:
    bool init() override;

    void update(float fixedDt) override;

    void render(float alpha) override;

    void shutdown() override;

  private:
    void renderUi();

    Camera3D camera_{};
    bool orbitalControl_ = false;
    bool manualControl_ = false;
    bool manualCapturing_ = false;
    float cameraMoveSpeed_ = 5.0f;
    float cameraRotateSpeed_ = 0.003f;

    model_loader::Transform transform_{};
    bool drawWires_ = false;
    bool showBounds_ = false;

    model_loader::ModelScene scene_;
    char modelPath_[512]{};
    char texturePath_[512]{};

    bool showDemo_ = false;
};
} // namespace app
