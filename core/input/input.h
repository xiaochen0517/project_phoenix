#pragma once

#include "config/config.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace input {
// 按键标识 (纯逻辑, 与 raylib KeyboardKey 解耦)。
// 字母/数字连续排列, 便于与 raylib KEY_A / KEY_ZERO 做等差映射。
enum class Key {
    Unknown,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    D0,
    D1,
    D2,
    D3,
    D4,
    D5,
    D6,
    D7,
    D8,
    D9,
    Space,
    Enter,
    Escape,
    Tab,
    Backspace,
    LeftShift,
    RightShift,
    LeftCtrl,
    RightCtrl,
    LeftAlt,
    RightAlt,
    Up,
    Down,
    Left,
    Right,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
};

// 动作状态 (每固定 tick 采样一次)。
enum class ActionState {
    Idle,
    Pressed,
    Held,
    Released
};

// 绑定结果。
enum class BindResult {
    Ok,
    Conflict
};

// 水平面方向向量 (x/z)。
struct Vec2 {
    float x = 0.0f;
    float z = 0.0f;
};

// 键名 ↔ Key 双向转换 (纯函数)。未知名返回 Key::Unknown, 未知 Key 返回 "UNKNOWN"。
// 名称规范: 单字母大写 "W"、数字 "0".."9"、特殊键全大写 "SPACE"/"LEFT_SHIFT" 等。
Key key_from_name(const std::string& name);
std::string key_name(Key key);

// 全部可识别键 (采样枚举用), 按枚举值升序。
const std::vector<Key>& all_keys();

// 动作 → 移动方向 (纯函数, 按 roadmap §1.1: 北=-z, 东=+x, W=西北)。
// move_up/down/left/right 返回归一化向量, 其余返回 nullopt。
std::optional<Vec2> action_to_dir(const std::string& action);

// 绑定表: action ↔ key, 含同表静态冲突检测。
class BindingMap {
  public:
    // 绑定 action → key; 若 key 已被本表其他 action 占用且未 force, 返回 Conflict 并保留原绑定。
    BindResult bind(const std::string& action, Key key, bool force = false);

    // 解绑动作, 返回是否确实存在该 action。
    bool unbind(const std::string& action);

    // 动作 → 按键; 未绑定返回 nullopt。
    std::optional<Key> query(const std::string& action) const;

    // 按键 → 动作列表 (反向查询 / 冲突检测)。
    std::vector<std::string> actions_for(Key key) const;

    // 全部动作名。
    std::vector<std::string> actions() const;

  private:
    std::unordered_map<std::string, Key> action_to_key_;
    std::unordered_map<Key, std::vector<std::string>> key_to_actions_;
};

// 输入管理器: 命名上下文 + 活动上下文 + 采样状态机。
class Manager {
  public:
    // 获取/创建命名上下文 (按名幂等)。
    BindingMap& context(const std::string& name);
    bool has_context(const std::string& name) const;

    // 切换活动上下文 (需已存在), 成功返回 true。
    bool set_context(const std::string& name);
    const std::string& active_context() const;

    // 采样: 传入本 tick 按下的键集合, 输出各动作状态 (边沿/持续)。
    void sample(const std::vector<Key>& pressed);

    // 动作当前状态 (最近一次采样结果, 未采样过返回 Idle)。
    ActionState state(const std::string& action) const;

    // 活动上下文内查询动作绑定。
    std::optional<Key> query(const std::string& action) const;

  private:
    std::unordered_map<std::string, BindingMap> contexts_;
    std::string active_;
    std::unordered_map<std::string, ActionState> states_;
    std::unordered_map<std::string, bool> prev_down_;
};

// 从配置文档加载绑定: 读 default_context 与 contexts.*, 绑定到 Manager (未知键名跳过并告警)。
// 依赖 config::Document (纯头), 不依赖 raylib/sol2 (可无头测试)。
void load_bindings(Manager& manager, const config::Document& doc);
} // namespace input
