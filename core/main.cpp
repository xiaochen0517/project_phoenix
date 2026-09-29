#include <string>

#include "imgui.h"
#include "raylib.h"
#include "rlImGui.h"

#include "log/app_log.h"
#include "script/lua_runner.h"

int main() {
    constexpr int screenWidth = 800;
    constexpr int screenHeight = 600;

    app_log::init();

    const std::string message = lua_runner::eval_string(
        "return string.format('Hello from %s, 6 * 7 = %d', _VERSION, 6 * 7)");
    app_log::info("Lua result: " + message);

    InitWindow(screenWidth, screenHeight, "raylib basic window");
    SetTargetFPS(60);

    rlImGuiSetup(true);
    bool showDemo = false;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Welcome to raylib!", 200, 200, 40, DARKGRAY);
        DrawText(message.c_str(), 200, 260, 20, DARKGRAY);

        rlImGuiBegin();
        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(280, 140), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Phoenix")) {
            ImGui::Text("FPS: %d", GetFPS());
            ImGui::TextWrapped("%s", message.c_str());
            ImGui::Checkbox("Show ImGui demo window", &showDemo);
        }
        ImGui::End();
        if (showDemo) {
            ImGui::ShowDemoWindow(&showDemo);
        }
        rlImGuiEnd();

        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
    app_log::info("Window closed");
    return 0;
}
