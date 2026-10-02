#pragma once

#include "ecs/components.h"
#include "world/world.h"

namespace movement {
// 移动与碰撞纯逻辑核心: 输入方向驱动位移, 经 world 的 walkability + 边墙碰撞。
// 只依赖 ecs/components.h + world (均纯逻辑), 不依赖 raylib / sol2 / input, 可无头单测 (ADR-9)。

// 移动结果。
struct MoveResult {
    bool moved = false; // 本 tick 是否发生位移
};

// 沿水平方向 (dx, dy) 尝试移动 pos, 经 world 碰撞判定 (轴分离: 先 X 后 Y, 贴墙可滑动)。
// 碰撞: 目标 tile walkable + 跨格边墙; 移动后高度贴地 (pos.z = 层级 z * 层高 + tile 高度)。
// 纯函数, 无 raylib / 无 IO, 可无头单测。
MoveResult step(const world::World& world, ecs::Position& pos, ecs::Mover& mover, float dx, float dy, float dt);
} // namespace movement
