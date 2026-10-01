#pragma once

#include <string>

namespace asset_path {
// 解析资源路径: 依次尝试原始路径、逐级 "../" 前缀 (最多 4 层)、可执行文件所在目录,
// 兼容不同工作目录 (如 IDE 将工作目录设为 build 子目录)。未找到时返回原始路径, 交由加载器统一报错。
std::string resolve(const std::string& requested);
} // namespace asset_path
