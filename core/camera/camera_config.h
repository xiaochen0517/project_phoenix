#pragma once

#include "camera/camera.h"

#include <optional>
#include <string>
#include <vector>

namespace config {
class Document; // 前置声明（config::Document 为第一方 pimpl 类型，无第三方头泄漏）
} // namespace config

namespace camera {
// 相机配置条目（config/camera.json 的 cameras[] 元素）。
struct CameraEntry {
    std::string name;
    Params params; // type / distance / fovy 已写入（projection 由 create 按类型默认化）
    bool follow_enabled = false;
    float smoothing = 0.0f; // 仅在 follow_enabled 时有效（> 0）
};

// 相机配置（config/camera.json 整体）。
struct Config {
    std::vector<CameraEntry> cameras;
    std::string active; // 空 = 取第一个相机名（load_config 内已归一化为非空有效名）
};

// 解析 JSON 文档为相机配置；结构校验失败返回 std::nullopt（经 app_log::error 输出）。
// 约定（对齐需求 20261003-01 §2）：
//   cameras[]: name 必填非空; type 枚举 "isometric" | "isometric_ortho"（默认 isometric）;
//              distance / fovy 可选（默认 10 / 45）; follow.smoothing 可选（缺失 = 不跟随, 必须 > 0）
//   active: 可选（缺失 = 第一个相机名; 指向不存在的相机名时告警并回退到第一个）
std::optional<Config> load_config(const config::Document& doc);
} // namespace camera
