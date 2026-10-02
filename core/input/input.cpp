#include "input/input.h"

#include "log/app_log.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace input {
Key key_from_name(const std::string& name) {
    if (name.size() == 1) {
        const char c = name[0];
        if (c >= 'A' && c <= 'Z') {
            return static_cast<Key>(static_cast<int>(Key::A) + (c - 'A'));
        }
        if (c >= 'a' && c <= 'z') {
            return static_cast<Key>(static_cast<int>(Key::A) + (c - 'a'));
        }
        if (c >= '0' && c <= '9') {
            return static_cast<Key>(static_cast<int>(Key::D0) + (c - '0'));
        }
    }
    static const std::unordered_map<std::string, Key> special = {
        {"SPACE", Key::Space},
        {"ENTER", Key::Enter},
        {"ESCAPE", Key::Escape},
        {"TAB", Key::Tab},
        {"BACKSPACE", Key::Backspace},
        {"LEFT_SHIFT", Key::LeftShift},
        {"RIGHT_SHIFT", Key::RightShift},
        {"LEFT_CTRL", Key::LeftCtrl},
        {"RIGHT_CTRL", Key::RightCtrl},
        {"LEFT_ALT", Key::LeftAlt},
        {"RIGHT_ALT", Key::RightAlt},
        {"UP", Key::Up},
        {"DOWN", Key::Down},
        {"LEFT", Key::Left},
        {"RIGHT", Key::Right},
        {"F1", Key::F1},
        {"F2", Key::F2},
        {"F3", Key::F3},
        {"F4", Key::F4},
        {"F5", Key::F5},
        {"F6", Key::F6},
        {"F7", Key::F7},
        {"F8", Key::F8},
        {"F9", Key::F9},
        {"F10", Key::F10},
        {"F11", Key::F11},
        {"F12", Key::F12},
    };
    const auto it = special.find(name);
    return it != special.end() ? it->second : Key::Unknown;
}

std::string key_name(Key key) {
    const int v = static_cast<int>(key);
    if (v >= static_cast<int>(Key::A) && v <= static_cast<int>(Key::Z)) {
        return std::string(1, static_cast<char>('A' + (v - static_cast<int>(Key::A))));
    }
    if (v >= static_cast<int>(Key::D0) && v <= static_cast<int>(Key::D9)) {
        return std::string(1, static_cast<char>('0' + (v - static_cast<int>(Key::D0))));
    }
    switch (key) {
    case Key::Space:
        return "SPACE";
    case Key::Enter:
        return "ENTER";
    case Key::Escape:
        return "ESCAPE";
    case Key::Tab:
        return "TAB";
    case Key::Backspace:
        return "BACKSPACE";
    case Key::LeftShift:
        return "LEFT_SHIFT";
    case Key::RightShift:
        return "RIGHT_SHIFT";
    case Key::LeftCtrl:
        return "LEFT_CTRL";
    case Key::RightCtrl:
        return "RIGHT_CTRL";
    case Key::LeftAlt:
        return "LEFT_ALT";
    case Key::RightAlt:
        return "RIGHT_ALT";
    case Key::Up:
        return "UP";
    case Key::Down:
        return "DOWN";
    case Key::Left:
        return "LEFT";
    case Key::Right:
        return "RIGHT";
    case Key::F1:
        return "F1";
    case Key::F2:
        return "F2";
    case Key::F3:
        return "F3";
    case Key::F4:
        return "F4";
    case Key::F5:
        return "F5";
    case Key::F6:
        return "F6";
    case Key::F7:
        return "F7";
    case Key::F8:
        return "F8";
    case Key::F9:
        return "F9";
    case Key::F10:
        return "F10";
    case Key::F11:
        return "F11";
    case Key::F12:
        return "F12";
    default:
        return "UNKNOWN";
    }
}

const std::vector<Key>& all_keys() {
    static const std::vector<Key> keys = [] {
        std::vector<Key> v;
        for (int i = static_cast<int>(Key::A); i <= static_cast<int>(Key::Z); ++i) {
            v.push_back(static_cast<Key>(i));
        }
        for (int i = static_cast<int>(Key::D0); i <= static_cast<int>(Key::D9); ++i) {
            v.push_back(static_cast<Key>(i));
        }
        for (const Key k :
             {Key::Space,      Key::Enter,    Key::Escape,    Key::Tab,     Key::Backspace, Key::LeftShift,
              Key::RightShift, Key::LeftCtrl, Key::RightCtrl, Key::LeftAlt, Key::RightAlt,  Key::Up,
              Key::Down,       Key::Left,     Key::Right,     Key::F1,      Key::F2,        Key::F3,
              Key::F4,         Key::F5,       Key::F6,        Key::F7,      Key::F8,        Key::F9,
              Key::F10,        Key::F11,      Key::F12}) {
            v.push_back(k);
        }
        return v;
    }();
    return keys;
}

std::optional<Vec2> action_to_dir(const std::string& action) {
    constexpr float k = 0.70710678f; // 1/√2, 对角单位向量分量
    if (action == "move_up") {
        return Vec2{-k, -k};
    }
    if (action == "move_down") {
        return Vec2{k, k};
    }
    if (action == "move_left") {
        return Vec2{-k, k};
    }
    if (action == "move_right") {
        return Vec2{k, -k};
    }
    return std::nullopt;
}

BindResult BindingMap::bind(const std::string& action, Key key, bool force) {
    const auto owners_it = key_to_actions_.find(key);
    if (owners_it != key_to_actions_.end() && !owners_it->second.empty()) {
        const std::vector<std::string>& owners = owners_it->second;
        // 仅被当前 action 自身占用 → 视为重复绑定同一键, 直接成功。
        if (!(owners.size() == 1 && owners[0] == action)) {
            if (!force) {
                return BindResult::Conflict;
            }
            // force: 顶替占用该键的其他动作。
            for (const std::string& owner : owners) {
                action_to_key_.erase(owner);
            }
            owners_it->second.clear();
        }
    }

    // 若 action 之前绑定了别的键, 先清理旧反向索引。
    const auto old_it = action_to_key_.find(action);
    if (old_it != action_to_key_.end() && old_it->second != key) {
        auto& old_owners = key_to_actions_[old_it->second];
        old_owners.erase(std::remove(old_owners.begin(), old_owners.end(), action), old_owners.end());
    }

    action_to_key_[action] = key;
    key_to_actions_[key].push_back(action);
    return BindResult::Ok;
}

bool BindingMap::unbind(const std::string& action) {
    const auto it = action_to_key_.find(action);
    if (it == action_to_key_.end()) {
        return false;
    }
    const Key key = it->second;
    action_to_key_.erase(it);
    auto& owners = key_to_actions_[key];
    owners.erase(std::remove(owners.begin(), owners.end(), action), owners.end());
    return true;
}

std::optional<Key> BindingMap::query(const std::string& action) const {
    const auto it = action_to_key_.find(action);
    if (it == action_to_key_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<std::string> BindingMap::actions_for(Key key) const {
    const auto it = key_to_actions_.find(key);
    if (it == key_to_actions_.end()) {
        return {};
    }
    return it->second;
}

std::vector<std::string> BindingMap::actions() const {
    std::vector<std::string> result;
    result.reserve(action_to_key_.size());
    for (const auto& entry : action_to_key_) {
        result.push_back(entry.first);
    }
    return result;
}

BindingMap& Manager::context(const std::string& name) {
    return contexts_[name];
}

bool Manager::has_context(const std::string& name) const {
    return contexts_.find(name) != contexts_.end();
}

bool Manager::set_context(const std::string& name) {
    if (contexts_.find(name) == contexts_.end()) {
        return false;
    }
    active_ = name;
    return true;
}

const std::string& Manager::active_context() const {
    return active_;
}

void Manager::sample(const std::vector<Key>& pressed) {
    std::unordered_map<Key, bool> down;
    for (const Key key : pressed) {
        down[key] = true;
    }

    const auto ctx_it = contexts_.find(active_);
    if (ctx_it == contexts_.end()) {
        return;
    }
    const BindingMap& bindings = ctx_it->second;

    for (const std::string& action : bindings.actions()) {
        const std::optional<Key> key = bindings.query(action);
        const bool now_down = key.has_value() && down.count(*key) > 0;
        const bool prev = prev_down_[action];
        if (!prev && now_down) {
            states_[action] = ActionState::Pressed;
        } else if (prev && now_down) {
            states_[action] = ActionState::Held;
        } else if (prev && !now_down) {
            states_[action] = ActionState::Released;
        } else {
            states_[action] = ActionState::Idle;
        }
        prev_down_[action] = now_down;
    }
}

ActionState Manager::state(const std::string& action) const {
    const auto it = states_.find(action);
    return it != states_.end() ? it->second : ActionState::Idle;
}

std::optional<Key> Manager::query(const std::string& action) const {
    const auto it = contexts_.find(active_);
    if (it == contexts_.end()) {
        return std::nullopt;
    }
    return it->second.query(action);
}

void load_bindings(Manager& manager, const config::Document& doc) {
    const std::string default_context = doc.get_string("default_context", "");

    for (const std::string& context_name : doc.keys("contexts")) {
        BindingMap& bindings = manager.context(context_name);
        const std::string context_path = "contexts." + context_name;
        for (const std::string& action : doc.keys(context_path)) {
            const std::string key_string = doc.get_string(context_path + "." + action, "");
            if (key_string.empty()) {
                continue;
            }
            const Key key = key_from_name(key_string);
            if (key == Key::Unknown) {
                app_log::warn("input: unknown key name '" + key_string + "' for action '" + action + "'");
                continue;
            }
            if (bindings.bind(action, key) == BindResult::Conflict) {
                app_log::warn("input: key conflict in context '" + context_name + "' for action '" + action + "'");
            }
        }
    }

    if (!default_context.empty() && manager.has_context(default_context)) {
        manager.set_context(default_context);
    }
}
} // namespace input
