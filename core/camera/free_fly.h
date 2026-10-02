#pragma once

#include "imgui.h"
#include "raylib.h"
#include "rcamera.h"

namespace camera {
// 自由飞行相机 (编辑器风格): WASD 前后左右 (以视角朝向为准), Space/Ctrl 升降, 右键按住捕获鼠标并旋转视角。
// 复用 raylib rcamera.h 的 CameraMoveForward/MoveRight/MoveUp/Yaw/Pitch 原语。
// capturing 为捕获状态 (调用方持有), 捕获期间置位 ImGuiConfigFlags_NoMouseCursorChange 避免与 rlImGui 光标管理冲突。
inline void update_free_fly(Camera3D& camera, float moveSpeed, float rotateSpeed, bool& capturing) {
    ImGuiIO& io = ImGui::GetIO();
    const float dist = moveSpeed * GetFrameTime();

    if (!io.WantCaptureKeyboard) {
        if (IsKeyDown(KEY_W)) {
            CameraMoveForward(&camera, dist, false);
        }
        if (IsKeyDown(KEY_S)) {
            CameraMoveForward(&camera, -dist, false);
        }
        if (IsKeyDown(KEY_D)) {
            CameraMoveRight(&camera, dist, false);
        }
        if (IsKeyDown(KEY_A)) {
            CameraMoveRight(&camera, -dist, false);
        }
        if (IsKeyDown(KEY_SPACE)) {
            CameraMoveUp(&camera, dist);
        }
        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            CameraMoveUp(&camera, -dist);
        }
    }

    const bool wantCapture = !io.WantCaptureMouse && IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    bool applyRotation = wantCapture;

    // rlImGui 每帧会按 ImGui 光标形状调用 ShowCursor/HideCursor, 与捕获逻辑冲突;
    // 捕获期间置位 NoMouseCursorChange 独占光标控制, 释放时恢复。
    if (wantCapture && !capturing) {
        capturing = true;
        DisableCursor();
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        applyRotation = false; // DisableCursor 会把鼠标重置到中心, 捕获首帧 delta 是跳变, 跳过
    } else if (!wantCapture && capturing) {
        capturing = false;
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
        EnableCursor();
    }

    if (applyRotation) {
        const Vector2 delta = GetMouseDelta();
        CameraYaw(&camera, -delta.x * rotateSpeed, false);
        CameraPitch(&camera, -delta.y * rotateSpeed, true, false, false);
    }
}
} // namespace camera
