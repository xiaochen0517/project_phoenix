#pragma once

#include <string>

namespace sol2_bridge {

// 运行 sol2 + LuaJIT 绑定自检, 返回逐项 PASS/FAIL 与统计摘要 (多行文本)。
std::string self_test();

}
