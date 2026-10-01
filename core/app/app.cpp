#include "app/app.h"

#include "app/game.h"
#include "imgui.h"
#include "log/app_log.h"
#include "raylib.h"
#include "rlImGui.h"

#include <algorithm>
#include <string>
#include <utility>

namespace {
// 2K (QHD) 默认窗口
constexpr int kScreenWidth = 2560;
constexpr int kScreenHeight = 1440;
constexpr const char* kWindowTitle = "Project Phoenix - 模型查看器 (2K)";

// 固定逻辑步长 (30Hz) 与帧时间上限 (防止卡顿后累加器死亡螺旋)。
constexpr double kFixedDt = 1.0 / 30.0;
constexpr double kMaxFrameTime = 0.25;

// ImGui 默认字体 ProggyClean 为位图字体, 仅含 ASCII/Latin 字形, 中文会渲染为 '?'。
// 这里在字体图集首次构建前加载系统 CJK 字体并设为默认字体 (含 Latin + CJK 字形),
// 并统一放大到 16px 以适配 2K 窗口。
void setupImGui() {
    rlImGuiBeginInitImGui(); // 创建 ImGui 上下文 + 添加默认字体
    ImGui::StyleColorsDark();

    constexpr float kFontSize = 16.0f;

    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig fontConfig;
    fontConfig.PixelSnapH = true;

    const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesChineseFull();

    const char* fontPath = nullptr;
    if (FileExists("C:/Windows/Fonts/msyh.ttc")) {
        fontPath = "C:/Windows/Fonts/msyh.ttc";
    } else if (FileExists("C:/Windows/Fonts/simhei.ttf")) {
        fontPath = "C:/Windows/Fonts/simhei.ttf";
    }

    ImFont* font = nullptr;
    if (fontPath != nullptr) {
        font = io.Fonts->AddFontFromFileTTF(fontPath, kFontSize, &fontConfig, glyphRanges);
    }

    if (font != nullptr) {
        io.FontDefault = font;
        app_log::info(std::string("ImGui CJK font loaded: ") + fontPath);
    } else {
        app_log::warn("未找到可用中文字体 (msyh.ttc / simhei.ttf), 中文可能显示为 '?'");
    }

    rlImGuiEndInitImGui(); // 合并 FontAwesome 图标、初始化后端
}
} // namespace

namespace app {
App::App(std::unique_ptr<Game> game) : game_(std::move(game)) {}

int App::run() {
    if (!game_) {
        app_log::error("App::run: game is null");
        return 1;
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(kScreenWidth, kScreenHeight, kWindowTitle);
    SetTargetFPS(60);
    setupImGui();

    if (!game_->init()) {
        app_log::error("App::run: game init failed");
        rlImGuiShutdown();
        CloseWindow();
        return 1;
    }

    // 固定 tick 累加器: 逻辑以 30Hz 固定步长推进, 渲染每帧一次, 用 alpha 做插值。
    double accumulator = 0.0;
    double elapsed = 0.0;
    uint64_t tickCount = 0;
    uint64_t lastTickCount = 0;

    while (!WindowShouldClose()) {
        const double frameTime = std::min<double>(GetFrameTime(), kMaxFrameTime);
        accumulator += frameTime;
        elapsed += frameTime;

        while (accumulator >= kFixedDt) {
            game_->update(static_cast<float>(kFixedDt));
            accumulator -= kFixedDt;
            ++tickCount;
        }

        const float alpha = static_cast<float>(accumulator / kFixedDt);
        game_->render(alpha);

        // 每秒输出一次逻辑推进统计, 供人工核对「30Hz 推进」。
        if (elapsed >= 1.0) {
            app_log::info("fixed tick: total=" + std::to_string(tickCount) +
                          ", rate=" + std::to_string(tickCount - lastTickCount) + "/s");
            lastTickCount = tickCount;
            elapsed -= 1.0;
        }
    }

    game_->shutdown();

    rlImGuiShutdown();
    CloseWindow();
    app_log::info("Window closed");
    return 0;
}
} // namespace app
