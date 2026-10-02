#pragma once

#include "config/config.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace world {
// 世界与地图 (纯逻辑核心, 不依赖 raylib / sol2 / ecs)。
// 3D 世界坐标 (x, y, z): y 为高度 (向上), 移动发生在 x/z 水平面。
// 北 = -z, 东 = +x, 南 = +z, 西 = -x。tile 坐标为整数 (x, z)。

struct TileCoord {
    std::int32_t x = 0;
    std::int32_t z = 0;
};

struct ChunkCoord {
    std::int32_t x = 0;
    std::int32_t z = 0;
};

// 格的四条边方向: n = -z, s = +z, e = +x, w = -x (全项目统一)。
enum class Side {
    North,
    East,
    South,
    West
};

// 楼梯方向 (None = 非楼梯)。
enum class Stairs {
    None,
    Up,
    Down
};

// tile 定义 (tile_defs 一项)。
struct TileDef {
    std::int32_t id = 0;  // 数值 id, 地图格内引用
    std::string name;     // 显示名 (UI 用)
    bool walkable = true; // 整格是否可走
    float height = 0.0f;  // tile 的 y 高度
    float r = 0.5f;       // 渲染颜色 (P1 纯色渲染)
    float g = 0.5f;
    float b = 0.5f;
    std::string texture; // 纹理标识/路径 (空 = 无纹理); P1 定形不加载
    Stairs stairs = Stairs::None;
};

// 边墙: 格 (cell) 的 side 一侧有墙, 每面墙唯一表示 (不做双向冗余)。
struct Wall {
    TileCoord cell;
    Side side = Side::North;
};

// 单层地图。
struct Level {
    std::string id;
    std::int32_t width = 0;                    // x 方向格数
    std::int32_t depth = 0;                    // z 方向格数
    std::vector<std::int32_t> tiles;           // 数值 tile id, 长度 width*depth, x 优先 (i = z*width + x)
    std::vector<Wall> walls;                   // 边墙列表 (加载顺序, 供遍历/渲染)
    std::unordered_set<std::int64_t> wall_set; // 边墙哈希集 (组合键, 查询加速)
};

// 坐标换算纯函数 (可单测, 含负数与边界)。
TileCoord world_to_tile(float wx, float wz, float tile_size);
void tile_to_world(const TileCoord& tile, float tile_size, float& wx, float& wz);
ChunkCoord tile_to_chunk(const TileCoord& tile, std::int32_t chunk_size);
TileCoord chunk_to_tile(const ChunkCoord& chunk, std::int32_t chunk_size);

// 地图: 从 config::Document 加载, 提供 tile 定义 / 层级 / walkability / 边墙 / 楼梯查询。
class World {
  public:
    // 从已解析的 config::Document 加载; 失败返回 nullopt (经 app_log::error 报错)。
    static std::optional<World> load(const config::Document& doc);

    // tile 定义 (按数值 id)。
    const TileDef* tile_def(std::int32_t id) const;

    // 层级。
    std::int32_t level_count() const;
    const Level* level(std::int32_t index) const;
    const Level* level(const std::string& id) const;

    // 查询 (level 越界/坐标越界返回安全默认值, 不崩溃)。
    bool is_walkable(std::int32_t level, const TileCoord& tile) const;
    bool has_wall(std::int32_t level, const TileCoord& tile, Side side) const;
    const TileDef* tile_def_at(std::int32_t level, const TileCoord& tile) const;
    Stairs stairs_at(std::int32_t level, const TileCoord& tile) const;

    // 全局参数。
    float tile_size() const;
    std::int32_t chunk_size() const;
    std::int32_t width(std::int32_t level) const; // x 方向格数 (越界返回 0)
    std::int32_t depth(std::int32_t level) const; // z 方向格数 (越界返回 0)

  private:
    const Level* level_or_null(std::int32_t level) const;
    bool in_bounds(const Level& lvl, const TileCoord& tile) const;
    std::int32_t tile_id_at(const Level& lvl, const TileCoord& tile) const;

    float tile_size_ = 1.0f;
    std::int32_t chunk_size_ = 16;
    std::unordered_map<std::int32_t, TileDef> tile_defs_; // id → def
    std::vector<Level> levels_;
    std::unordered_map<std::string, std::int32_t> level_index_; // level id → index
};

// 边方向 ↔ 字符串 (JSON/Lua 用): "n"/"e"/"s"/"w"。未识别返回 nullopt。
std::optional<Side> side_from_string(const std::string& s);
std::string side_to_string(Side side);
} // namespace world
