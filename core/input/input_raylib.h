#pragma once

#include "input/input.h"
#include "raylib.h"

namespace input {
// Key → raylib KeyboardKey (字母/数字等差映射, 特殊键查表)。仅运行侧 include (需 raylib)。
inline int to_raylib_key(Key key) {
    const int v = static_cast<int>(key);
    if (v >= static_cast<int>(Key::A) && v <= static_cast<int>(Key::Z)) {
        return KEY_A + (v - static_cast<int>(Key::A));
    }
    if (v >= static_cast<int>(Key::D0) && v <= static_cast<int>(Key::D9)) {
        return KEY_ZERO + (v - static_cast<int>(Key::D0));
    }
    switch (key) {
    case Key::Space:
        return KEY_SPACE;
    case Key::Enter:
        return KEY_ENTER;
    case Key::Escape:
        return KEY_ESCAPE;
    case Key::Tab:
        return KEY_TAB;
    case Key::Backspace:
        return KEY_BACKSPACE;
    case Key::LeftShift:
        return KEY_LEFT_SHIFT;
    case Key::RightShift:
        return KEY_RIGHT_SHIFT;
    case Key::LeftCtrl:
        return KEY_LEFT_CONTROL;
    case Key::RightCtrl:
        return KEY_RIGHT_CONTROL;
    case Key::LeftAlt:
        return KEY_LEFT_ALT;
    case Key::RightAlt:
        return KEY_RIGHT_ALT;
    case Key::Up:
        return KEY_UP;
    case Key::Down:
        return KEY_DOWN;
    case Key::Left:
        return KEY_LEFT;
    case Key::Right:
        return KEY_RIGHT;
    case Key::F1:
        return KEY_F1;
    case Key::F2:
        return KEY_F2;
    case Key::F3:
        return KEY_F3;
    case Key::F4:
        return KEY_F4;
    case Key::F5:
        return KEY_F5;
    case Key::F6:
        return KEY_F6;
    case Key::F7:
        return KEY_F7;
    case Key::F8:
        return KEY_F8;
    case Key::F9:
        return KEY_F9;
    case Key::F10:
        return KEY_F10;
    case Key::F11:
        return KEY_F11;
    case Key::F12:
        return KEY_F12;
    default:
        return KEY_NULL;
    }
}

// 采样当前帧所有按下的键 (raylib IsKeyDown), 转成 Key 集合。
// 每固定 tick 开头调用一次, 结果传给 Manager::sample。
inline std::vector<Key> sample_pressed_keys() {
    std::vector<Key> result;
    for (const Key key : all_keys()) {
        if (IsKeyDown(to_raylib_key(key))) {
            result.push_back(key);
        }
    }
    return result;
}
} // namespace input
