#pragma once

#include <cstdint>

namespace ecs {
// 正式组件 schema: 移动 / 渲染 / 相机跟随共同依赖的基础组件。
// 纯数据 struct, 供 ecs::Registry::attach<T> / view<T> 实例化 (entt 模板在知道 T 完整定义处实例化)。
// 本头仅依赖标准库, 可无头单测。

// 位置组件 (地图坐标: x = 东, y = 南为水平面, z = 高度向上)。
struct Position {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// 移动组件: speed = 每秒世界单位 (1 单位 = 1 米), z = 当前层级 (0 = 地面, 正 = 上层, 负 = 地下室)。
struct Mover {
    float speed = 3.0f;
    std::int32_t z = 0;
};
} // namespace ecs
