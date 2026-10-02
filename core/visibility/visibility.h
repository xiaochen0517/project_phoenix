#pragma once

#include "world/world.h"

#include <cstdint>
#include <unordered_map>

namespace visibility {
// 渲染遮挡判定 (纯逻辑): 规则 A (高层溶解) + 规则 B (同层墙透明)。
// 只依赖 world/ 与标准库, 不依赖 raylib / sol2 / ecs (对齐 ADR-9), 可无头单测。
// 视角约定: 等距相机固定位于玩家东南上方; 建筑位于玩家东南象限 (dx >= 0 && dy >= 0) 时处于相机前侧, 会截断视线。

// 玩家视角: 水平世界坐标 + 所在层 (z)。
struct View {
    float x = 0.0f;
    float y = 0.0f;
    std::int32_t z = 0;
};

// 每格墙裁剪位 (per-cell): bit0 = 南, bit1 = 东, bit2 = 北, bit3 = 西。
inline constexpr std::uint8_t kCutSouth = 0x1;
inline constexpr std::uint8_t kCutEast = 0x2;
inline constexpr std::uint8_t kCutNorth = 0x4;
inline constexpr std::uint8_t kCutWest = 0x8;

class Culler {
  public:
    // 每固定 tick 重算: 缓存玩家位置, 执行规则 A/B。
    void update(const world::World& world, const View& view);

    // 规则 A: 所有更高层 (z > 玩家层) 是否整层溶解 (不渲染)。
    bool upper_dissolved() const {
        return upper_dissolved_;
    }

    // 规则 B: 墙是否隐藏 (仅玩家层墙会置位; 其他层墙天然返回 false, 层级处理由调用方负责)。
    bool is_wall_hidden(const world::Wall& wall) const;

  private:
    // 规则 A: 屋内 / 建筑位于玩家东南 (相机前侧) 两触发。
    void compute_upper_dissolve(const world::World& world);
    // 规则 B: 屋内南北墙线整排置位; 屋外建筑位于玩家东南时当前层全部墙置位。
    void compute_wall_cutaway(const world::World& world);
    // 屋内: 沿玩家行/列 (含相邻行列, 覆盖 1 格门洞) 找最近的南墙线 / 东墙线; 返回墙线世界坐标。
    bool find_wall_line(const world::World& world, world::Side side, float& line) const;
    // 玩家层是否存在建筑内 tile 位于玩家东南象限 (dx >= 0 && dy >= 0), 即建筑处于相机前侧。
    bool has_building_southeast(const world::World& world) const;

    bool upper_dissolved_ = false;
    bool building_se_ = false; // 建筑位于玩家东南 (相机前侧) 的触发标志
    std::int32_t player_z_ = 0;
    world::TileCoord player_tile_{};
    std::unordered_map<std::int64_t, std::uint8_t> wall_flags_; // tile_key(cell) -> 裁剪位
};
} // namespace visibility
