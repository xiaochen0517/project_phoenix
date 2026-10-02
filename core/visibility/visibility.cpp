#include "visibility/visibility.h"

namespace visibility {
namespace {
constexpr std::int32_t kScanFallback = 512; // 层级未声明 width/depth 时的扫描上限

// side → 裁剪位 (仅玩家层墙会置位)。
std::uint8_t side_bit(world::Side side) {
    switch (side) {
    case world::Side::South:
        return kCutSouth;
    case world::Side::East:
        return kCutEast;
    case world::Side::North:
        return kCutNorth;
    case world::Side::West:
        return kCutWest;
    }
    return 0;
}
} // namespace

void Culler::update(const world::World& world, const View& view) {
    player_z_ = view.z;
    player_tile_ = world::world_to_tile(view.x, view.y, world.tile_size());

    // 建筑是否位于玩家东南 (相机前侧): 玩家层存在建筑内 tile 且位于玩家东南象限。
    building_se_ = has_building_southeast(world);

    wall_flags_.clear();
    compute_upper_dissolve(world);
    compute_wall_cutaway(world);
}

void Culler::compute_upper_dissolve(const world::World& world) {
    upper_dissolved_ = false;

    // ① 屋内: 玩家 tile 标记为建筑内 → 溶解所有更高层。
    if (world.is_interior(player_z_, player_tile_)) {
        upper_dissolved_ = true;
        return;
    }

    // ② 玩家在建筑西北方 (建筑位于玩家东南、相机前侧): 建筑会遮挡角色,
    //    直接隐藏所有高于当前层级的内容 (不做距离判定)。
    if (building_se_) {
        upper_dissolved_ = true;
        return;
    }

    // ③ 玩家在建筑东南方 (建筑位于玩家西北、屏幕后方): 不进行任何溶解操作。
    //    后续加入角色视线 (LOS) 判定后, 若建筑开窗且视线可穿入, 再在此扩展对应的溶解逻辑; 当前不涉及。
}

void Culler::compute_wall_cutaway(const world::World& world) {
    const world::Level* lvl = world.level(player_z_);
    if (lvl == nullptr) {
        return;
    }

    if (world.is_interior(player_z_, player_tile_)) {
        // 屋内: 南墙线与东墙线整排置位 (含门洞两侧墙段); 北/西墙保持不透明。
        // 南/东墙位于玩家屏幕前侧 (相机在东南上方), 会遮挡视线; 北/西墙在屏幕后方, 不遮挡。
        float south_line = 0.0f;
        float east_line = 0.0f;
        const bool has_south = find_wall_line(world, world::Side::South, south_line);
        const bool has_east = find_wall_line(world, world::Side::East, east_line);
        if (!has_south && !has_east) {
            return;
        }
        const float tile_size = world.tile_size();
        for (const world::Wall& wall : lvl->walls) {
            if (wall.side == world::Side::South) {
                // 南墙墙线 = (cell.y + 1) × tile_size。
                if (has_south && static_cast<float>(wall.cell.y + 1) * tile_size == south_line) {
                    wall_flags_[world::tile_key(wall.cell.x, wall.cell.y)] |= kCutSouth;
                }
            } else if (wall.side == world::Side::East) {
                // 东墙墙线 = (cell.x + 1) × tile_size。
                if (has_east && static_cast<float>(wall.cell.x + 1) * tile_size == east_line) {
                    wall_flags_[world::tile_key(wall.cell.x, wall.cell.y)] |= kCutEast;
                }
            }
        }
    } else if (building_se_) {
        // 玩家在建筑西北方 (建筑位于玩家东南、相机前侧): 建筑会遮挡角色,
        // 将当前层级的所有墙设置为透明。
        for (const world::Wall& wall : lvl->walls) {
            wall_flags_[world::tile_key(wall.cell.x, wall.cell.y)] |= side_bit(wall.side);
        }
    }
    // else: 玩家在建筑东南方 (建筑位于玩家西北、屏幕后方): 不进行任何透明操作。
    // 后续加入角色视线 (LOS) 判定后, 若建筑开窗且视线可穿入, 再在此扩展对应的透明逻辑; 当前不涉及。
}

bool Culler::find_wall_line(const world::World& world, world::Side side, float& line) const {
    const world::Level* lvl = world.level(player_z_);
    if (lvl == nullptr) {
        return false;
    }
    // 扫描上限: 南向沿 depth 方向, 东向沿 width 方向; 层级未声明时用固定上限。
    std::int32_t scan_max = kScanFallback;
    if (side == world::Side::South && lvl->depth > 0) {
        scan_max = lvl->depth;
    } else if (side == world::Side::East && lvl->width > 0) {
        scan_max = lvl->width;
    }

    // 从玩家行/列起逐格扫描; 每步检查玩家行列及相邻行列 (覆盖 1 格宽门洞)。
    // has_wall 为对称查询, 墙无论存储在相邻格哪一侧都能命中。
    if (side == world::Side::South) {
        for (std::int32_t y = player_tile_.y; y < player_tile_.y + scan_max; ++y) {
            for (std::int32_t dx = -1; dx <= 1; ++dx) {
                if (world.has_wall(player_z_, {player_tile_.x + dx, y}, world::Side::South)) {
                    line = (static_cast<float>(y) + 1.0f) * world.tile_size();
                    return true;
                }
            }
        }
    } else {
        for (std::int32_t x = player_tile_.x; x < player_tile_.x + scan_max; ++x) {
            for (std::int32_t dy = -1; dy <= 1; ++dy) {
                if (world.has_wall(player_z_, {x, player_tile_.y + dy}, world::Side::East)) {
                    line = (static_cast<float>(x) + 1.0f) * world.tile_size();
                    return true;
                }
            }
        }
    }
    return false;
}

bool Culler::has_building_southeast(const world::World& world) const {
    const world::Level* lvl = world.level(player_z_);
    if (lvl == nullptr) {
        return false;
    }
    for (const std::int64_t key : lvl->interior_tiles) {
        std::int32_t x = 0;
        std::int32_t y = 0;
        world::tile_key_decode(key, x, y);
        if (x >= player_tile_.x && y >= player_tile_.y) {
            return true;
        }
    }
    return false;
}

bool Culler::is_wall_hidden(const world::Wall& wall) const {
    // 仅玩家层墙会置位; 其他层墙无条目 → false。
    const auto it = wall_flags_.find(world::tile_key(wall.cell.x, wall.cell.y));
    if (it == wall_flags_.end()) {
        return false;
    }
    return (it->second & side_bit(wall.side)) != 0;
}
} // namespace visibility
