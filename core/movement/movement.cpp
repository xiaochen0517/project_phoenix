#include "movement/movement.h"

#include <cmath>

namespace movement {
MoveResult step(const world::World& world, ecs::Position& pos, ecs::Mover& mover, float dx, float dy, float dt) {
    MoveResult result;

    // 归一化水平方向; 无有效输入则直接返回。
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len <= 0.0001f) {
        return result;
    }
    const float nx = dx / len;
    const float ny = dy / len;

    const float tile_size = world.tile_size();
    const std::int32_t z = mover.z;
    const float step_len = mover.speed * dt;

    // 轴分离: 先 X 后 Y, 便于贴墙滑动。
    // 墙碰撞仅在跨格 (target != from) 时检查, 使玩家能贴墙站立 (墙两侧一圈 tile 可正常进入与移动)。
    const float step_x = nx * step_len;
    if (std::fabs(step_x) > 0.0f) {
        const world::TileCoord from = world::world_to_tile(pos.x, pos.y, tile_size);
        const float new_x = pos.x + step_x;
        const world::TileCoord target = world::world_to_tile(new_x, pos.y, tile_size);
        const world::Side side = step_x > 0.0f ? world::Side::East : world::Side::West;
        const bool crossing = target.x != from.x;
        if (world.is_walkable(z, target) && (!crossing || !world.has_wall(z, from, side))) {
            pos.x = new_x;
            result.moved = true;
        }
    }

    const float step_y = ny * step_len;
    if (std::fabs(step_y) > 0.0f) {
        // X 轴可能已更新 pos.x, 此处用最新位置重算当前格。
        const world::TileCoord from = world::world_to_tile(pos.x, pos.y, tile_size);
        const float new_y = pos.y + step_y;
        const world::TileCoord target = world::world_to_tile(pos.x, new_y, tile_size);
        const world::Side side = step_y > 0.0f ? world::Side::South : world::Side::North;
        const bool crossing = target.y != from.y;
        if (world.is_walkable(z, target) && (!crossing || !world.has_wall(z, from, side))) {
            pos.y = new_y;
            result.moved = true;
        }
    }

    // 高度贴地: 层级基准高度 + tile 高度。
    const world::TileCoord cur = world::world_to_tile(pos.x, pos.y, tile_size);
    const world::TileDef* def = world.tile_def_at(z, cur);
    if (def != nullptr) {
        pos.z = static_cast<float>(z) * world.level_height() + def->height;
    }

    return result;
}
} // namespace movement
