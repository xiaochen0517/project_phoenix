#pragma once

#include "app/game.h"

#include <memory>
#include <string>
#include <vector>

namespace app {
// 场景切换载体: 实现 Game, 持有多个命名场景并委托给活动场景。
// 负责渲染帧所有权 (Begin/EndDrawing + rlImGuiBegin/End) 与统一顶部场景选择器。
// 场景名采用「大类/场景名」两级命名 (如 "debug/model_viewer"、"main_game/demo")。
class SceneHost : public Game {
  public:
    SceneHost();

    // 注册场景; 名称需唯一 (「大类/场景名」), 重复名返回 false 不覆盖。
    bool add(const std::string& name, std::unique_ptr<Game> scene);

    // 切换到指定场景 (懒初始化: 首次切换时才 init); 成功返回 true。
    bool switch_to(const std::string& name);

    // 当前活动场景名 (无活动场景返回空串)。
    const std::string& active_name() const;

    // Game 接口。
    bool init() override;
    void update(float fixedDt) override;
    void render(float alpha) override;
    void renderUi() override;
    void shutdown() override;

  private:
    struct Entry {
        std::string name;
        std::unique_ptr<Game> scene;
        bool initialized = false;
    };

    Entry* find(const std::string& name);
    const Entry* find(const std::string& name) const;
    void renderSceneSelector();

    std::vector<Entry> scenes_;
    std::string active_;
};
} // namespace app
