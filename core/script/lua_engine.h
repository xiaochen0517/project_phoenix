#pragma once

#include <memory>
#include <string>
#include <variant>

struct lua_State; // 前置声明, 不 include lua.h / sol2 (ADR-5)

namespace lua_engine {
// 基础类型值包装: 隔离 sol2 类型, 应用层不直接接触 sol::object (ADR-5)。
class Value {
  public:
    Value() = default;

    bool is_nil() const;

    bool is_bool() const;

    bool as_bool() const;

    bool is_number() const;

    double as_number() const;

    bool is_string() const;

    std::string as_string() const;

  private:
    friend class Engine;
    std::variant<std::monostate, bool, double, std::string> data_;
};

class Engine {
  public:
    Engine();

    ~Engine();

    Engine(const Engine&) = delete;

    Engine& operator=(const Engine&) = delete;

    // 加载并执行一个 .lua 文件; 出错经 app_log::error 输出并返回 false。
    bool do_file(const std::string& path);

    // 重新执行已加载的文件 (手动基础重载, 覆盖全局量; 函数级/自动监听热重载留到 P7)。
    bool reload(const std::string& path);

    // 调用全局函数并返回基础类型值; P0 仅支持无参调用, 参数/对象绑定在 P0-04 扩展。
    Value call_function(const std::string& name);

    // 供 api 绑定层使用: 返回内部 sol::state 的裸 lua_State* (sol2 头仅在 .cpp, ADR-5)。
    lua_State* raw_state();

  private:
    struct Impl; // pimpl: 持有 sol::state, 隔离 sol2 头 (ADR-5)
    std::unique_ptr<Impl> impl_;
};
} // namespace lua_engine
