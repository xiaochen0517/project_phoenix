#include "script/lua_engine.h"

#include "log/app_log.h"

#include <memory>
#include <sol/sol.hpp>
#include <string>
#include <variant>

namespace lua_engine {
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

// ---- Engine::Impl ----
struct Engine::Impl {
    sol::state lua;

    Impl() {
        // 必须用无参 open_libraries() (等价 luaL_openlibs), 才能把 jit 注册为全局变量。
        // 带参重载对 lib::jit 仅写 package.loaded, 不创建全局 jit, 会导致 jit.version 为 nil 而崩溃。
        lua.open_libraries();
    }
};

Engine::Engine() : impl_(std::make_unique<Impl>()) {}

Engine::~Engine() = default;

bool Engine::do_file(const std::string& path) {
    const sol::protected_function_result result = impl_->lua.safe_script_file(path, sol::script_pass_on_error);
    if (!result.valid()) {
        const sol::error err = result;
        app_log::error("lua do_file failed: " + path + " (" + err.what() + ")");
        return false;
    }
    return true;
}

bool Engine::reload(const std::string& path) {
    // P0 阶段语义与 do_file 一致 (重新执行文件 chunk, 覆盖全局量)。
    return do_file(path);
}

lua_State* Engine::raw_state() {
    return impl_->lua.lua_state();
}

Value Engine::call_function(const std::string& name) {
    Value out;

    const sol::protected_function fn = impl_->lua[name];
    if (!fn.valid()) {
        app_log::error("lua call_function: function not found: " + name);
        return out;
    }

    const sol::protected_function_result result = fn();
    if (!result.valid()) {
        const sol::error err = result;
        app_log::error("lua call_function failed: " + name + " (" + err.what() + ")");
        return out;
    }
    if (result.return_count() < 1) {
        return out;
    }

    const sol::object obj = result.get<sol::object>(0);
    switch (obj.get_type()) {
    case sol::type::boolean:
        out.data_ = obj.as<bool>();
        break;
    case sol::type::number:
        out.data_ = obj.as<double>();
        break;
    case sol::type::string:
        out.data_ = obj.as<std::string>();
        break;
    case sol::type::nil:
    case sol::type::none:
    default:
        break; // 保持 monostate (nil)
    }
    return out;
}
} // namespace lua_engine
