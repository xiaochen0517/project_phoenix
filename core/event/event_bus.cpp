#include "event/event_bus.h"

#include <algorithm>

namespace event {
// ---- Value ----
bool Value::is_nil() const {
    return std::holds_alternative<std::monostate>(data_);
}

bool Value::is_bool() const {
    return std::holds_alternative<bool>(data_);
}

bool Value::as_bool() const {
    return std::get<bool>(data_);
}

bool Value::is_number() const {
    return std::holds_alternative<double>(data_);
}

double Value::as_number() const {
    return std::get<double>(data_);
}

bool Value::is_string() const {
    return std::holds_alternative<std::string>(data_);
}

std::string Value::as_string() const {
    return std::get<std::string>(data_);
}

// ---- Bus ----
Bus::SubscriptionId Bus::on(const std::string& name, Handler handler) {
    const SubscriptionId id = ++next_id_;
    subscribers_[name].push_back(Entry{id, std::move(handler)});
    return id;
}

void Bus::off(const std::string& name, SubscriptionId id) {
    const auto it = subscribers_.find(name);
    if (it == subscribers_.end()) {
        return;
    }
    auto& entries = it->second;
    entries.erase(std::remove_if(entries.begin(), entries.end(), [id](const Entry& entry) { return entry.id == id; }),
                  entries.end());
}

void Bus::emit(const std::string& name, const Value& payload) {
    const auto it = subscribers_.find(name);
    if (it == subscribers_.end()) {
        return;
    }
    // 同步分发: 拷贝订阅列表快照, 避免订阅者在回调中增删订阅导致迭代器失效。
    const std::vector<Entry> snapshot = it->second;
    for (const Entry& entry : snapshot) {
        entry.handler(payload);
    }
}
} // namespace event
