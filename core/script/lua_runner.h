#pragma once

#include <string>

namespace lua_runner {
    // 执行一段 Lua 脚本, 返回其字符串结果; 出错时返回以 "error: " 开头的信息。
    std::string eval_string(const std::string &script);
}
