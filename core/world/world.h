#pragma once

#include "config/config.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace world {
// 世界与地图 (纯逻辑核心, 不依赖 raylib / sol2 / ecs)。
// 地图坐标: 水平面 (x, y), 层级 z (整数, 0 = 地面, 正 = 上层, 负 = 地下室)。
// 北 = -y, 东 = +x, 南 = +y, 西 = -x。层高 = 1 世界单位 (层级 z 的基准高度 = z)。

struct TileCoord {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct ChunkCoord {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

// 格的四条边方向: n = -y, s = +y, e = +x, w = -x (全项目统一)。
enum class Side {
    North,
    East,
    South,
    West
};

// tile 定义 (tile_defs 一项)。
struct TileDef {
    std::int32_t id = 0;  // 数值 id, 地图格内引用
    std::string name;     // 显示名 (UI 用)
    bool walkable = true; // 整格是否可走
    float height = 0.0f;  // tile 的高度 (相对本层基准高度)
    float r = 0.5f;       // 渲染颜色 (P1 纯色渲染)
    float g = 0.5f;
    float b = 0.5f;
    std::string texture; // 纹理标识/路径 (空 = 无纹理); P1 定形不加载
};

// 边墙: 格 (cell) 的 side 一侧有墙, 每面墙唯一表示 (不做双向冗余)。
struct Wall {
    TileCoord cell;
    Side side = Side::North;
};

// 单层地图。
struct Level {
    std::string id;
    std::int32_t width = 0; // 世界尺寸 (x 方向格数), 可选 (0 = 未声明)
    std::int32_t depth = 0; // 世界尺寸 (y 方向格数), 可选 (0 = 未声明)
    // 稀疏 tile 存储: (x,y) 组合键 → 数值 tile id, 仅存非空 tile; 未列出的格子 = 空 (void)。
    std::unordered_map<std::int64_t, std::int32_t> tiles;
    // 建筑内 tile 组合键 (interior=true 的 tile, 纯渲染遮挡用, 与 walkable 无关)。
    std::unordered_set<std::int64_t> interior_tiles;
    std::vector<Wall> walls;                   // 边墙列表 (加载顺序, 供遍历/渲染)
    std::unordered_set<std::int64_t> wall_set; // 边墙哈希集 (组合键, 查询加速)
};

// 坐标换算纯函数 (可单测, 含负数与边界)。
TileCoord world_to_tile(float wx, float wy, float tile_size);
void tile_to_world(const TileCoord& tile, float tile_size, float& wx, float& wy);
ChunkCoord tile_to_chunk(const TileCoord& tile, std::int32_t chunk_size);
TileCoord chunk_to_tile(const ChunkCoord& chunk, std::int32_t chunk_size);

// 稀疏 tile 组合键编解码 (供内部存储与渲染遍历): x 在高 32 位, y 在低 32 位。
std::int64_t tile_key(std::int32_t x, std::int32_t y);
void tile_key_decode(std::int64_t key, std::int32_t& x, std::int32_t& y);

// 地图: 从 config::Document 加载, 提供 tile 定义 / 层级 / walkability / 边墙查询。
class World {
  public:
    // 从已解析的 config::Document 加载; 失败返回 nullopt (经 app_log::error 报错)。
    static std::optional<World> load(const config::Document& doc);

    // tile 定义 (按数值 id)。
    const TileDef* tile_def(std::int32_t id) const;

    // 层级 (按 z 值)。
    std::int32_t level_count() const;
    const Level* level(std::int32_t z) const;
    const Level* level(const std::string& id) const;
    float level_height() const; // 层高 (1 世界单位)
    // 垂直缩放: z 轴 1 单位 = 渲染的多少单位 (默认 1.0)。
    float vertical_scale() const;
    std::vector<std::int32_t> level_zs() const; // 所有层级 z 值 (升序)

    // 查询 (层级/坐标不存在返回安全默认值, 不崩溃)。
    bool is_walkable(std::int32_t z, const TileCoord& tile) const;
    bool has_wall(std::int32_t z, const TileCoord& tile, Side side) const;
    const TileDef* tile_def_at(std::int32_t z, const TileCoord& tile) const;
    // 指定 tile 是否为建筑内部 (数据驱动, 层级/未声明/越界 → false)。
    bool is_interior(std::int32_t z, const TileCoord& tile) const;

    // 全局参数。
    float tile_size() const;
    std::int32_t chunk_size() const;
    std::int32_t width(std::int32_t z) const; // x 方向格数 (层级不存在返回 0)
    std::int32_t depth(std::int32_t z) const; // y 方向格数 (层级不存在返回 0)

  private:
    const Level* level_or_null(std::int32_t z) const;
    std::int32_t tile_id_at(const Level& lvl, const TileCoord& tile) const;

    float tile_size_ = 1.0f;
    std::int32_t chunk_size_ = 16;
    float level_height_ = 1.0f;
    float vertical_scale_ = 1.0f;
    std::unordered_map<std::int32_t, TileDef> tile_defs_;       // id → def
    std::map<std::int32_t, Level> levels_;                      // z → Level (有序, 支持负 z)
    std::unordered_map<std::string, std::int32_t> level_index_; // level id → z
};

// 边方向 ↔ 字符串 (JSON/Lua 用): "n"/"e"/"s"/"w"。未识别返回 nullopt。
std::optional<Side> side_from_string(const std::string& s);
std::string side_to_string(Side side);
} // namespace world
