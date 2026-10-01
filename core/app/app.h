#pragma once

#include <memory>

namespace app {
class Game;

// 应用外壳: 负责窗口 / ImGui 初始化、固定 tick 主循环与清理, 不依赖具体业务。
class App {
  public:
    explicit App(std::unique_ptr<Game> game);

    // 运行主循环直至窗口关闭; 返回进程退出码。
    int run();

  private:
    std::unique_ptr<Game> game_;
};
} // namespace app
