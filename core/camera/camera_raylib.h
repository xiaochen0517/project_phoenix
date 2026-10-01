#pragma once

#include "camera/camera.h"
#include "raylib.h"

namespace camera {
// 纯逻辑 Params → raylib Camera3D（字段直拷；调用方负责在需要时先 apply_isometric）。
// 本头仅运行侧使用（include raylib.h），测试目标不链接 raylib、不 include 本头。
inline Camera3D to_camera3d(const Params& params) {
    Camera3D c{};
    c.position = {params.pos[0], params.pos[1], params.pos[2]};
    c.target = {params.target[0], params.target[1], params.target[2]};
    c.up = {params.up[0], params.up[1], params.up[2]};
    c.fovy = params.fovy;
    c.projection = params.projection;
    return c;
}
} // namespace camera
