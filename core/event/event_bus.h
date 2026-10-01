#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace event {
// 载荷值: 收敛四类基础类型 (nil / bool / number / string), 语义对齐 lua_engine::Value。
class Value {
  public:
    Value() = default; // nil

    static Value nil() { return Value(); }

    static Value boolean(bool v) {
        Value out;
        out.data_ = v;
        return out;
    }

    static Value number(double v) {
        Value out;
        out.data_ = v;
        return out;
    }

    static Value string(std::string v) {
        Value out;
        out.data_ = std::move(v);
        return out;
    }

    bool is_nil() const;

    bool is_bool() const;

    bool as_bool() const;

    bool is_number() const;

    double as_number() const;

    bool is_string() const;

    std::string as_string() const;

  private:
    std::variant<std::monostate, bool, double, std::string> data_;
};

// 事件总线: 事件名 → 订阅列表, 同步分发。纯 C++, 不依赖 sol2 / Lua。
class Bus {
  public:
    using Handler = std::function<void(const Value&)>;
    using SubscriptionId = std::uint64_t;

    // 订阅事件, 返回订阅 id 供 off 精确移除。
    SubscriptionId on(const std::string& name, Handler handler);

    // 按 id 移除订阅。
    void off(const std::string& name, SubscriptionId id);

    // 同步分发事件到该事件名的所有订阅者。
    void emit(const std::string& name, const Value& payload = Value::nil());

  private:
    struct Entry {
        SubscriptionId id;
        Handler handler;
    };

    std::unordered_map<std::string, std::vector<Entry>> subscribers_;
    SubscriptionId next_id_ = 0;
};
} // namespace event
