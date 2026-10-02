#include "world/world.h"

#include "log/app_log.h"

#include <cmath>
#include <string>
#include <utility>

namespace world {
namespace {
// 组合键: cell.x(21bit) | cell.y(21bit) | side(2bit)。坐标范围 ±1048576 内唯一。
constexpr std::int64_t kCoordMask = 0x1FFFFF;

std::int64_t wall_key(std::int32_t x, std::int32_t y, Side side) {
    const std::int64_t kx = static_cast<std::int64_t>(x) & kCoordMask;
    const std::int64_t ky = static_cast<std::int64_t>(y) & kCoordMask;
    const std::int64_t ks = static_cast<std::int64_t>(side);
    return kx | (ky << 21) | (ks << 42);
}

Side opposite(Side side) {
    switch (side) {
    case Side::North:
        return Side::South;
    case Side::South:
        return Side::North;
    case Side::East:
        return Side::West;
    case Side::West:
        return Side::East;
    }
    return Side::North;
}

TileCoord neighbor(const TileCoord& tile, Side side) {
    switch (side) {
    case Side::North:
        return {tile.x, tile.y - 1};
    case Side::South:
        return {tile.x, tile.y + 1};
    case Side::East:
        return {tile.x + 1, tile.y};
    case Side::West:
        return {tile.x - 1, tile.y};
    }
    return tile;
}

std::int32_t floor_div(std::int32_t v, std::int32_t d) {
    const std::int32_t q = v / d;
    const std::int32_t r = v % d;
    return (r < 0) ? q - 1 : q;
}
} // namespace

TileCoord world_to_tile(float wx, float wy, float tile_size) {
    return {
        static_cast<std::int32_t>(std::floor(wx / tile_size)),
        static_cast<std::int32_t>(std::floor(wy / tile_size)),
    };
}

void tile_to_world(const TileCoord& tile, float tile_size, float& wx, float& wy) {
    wx = (static_cast<float>(tile.x) + 0.5f) * tile_size;
    wy = (static_cast<float>(tile.y) + 0.5f) * tile_size;
}

ChunkCoord tile_to_chunk(const TileCoord& tile, std::int32_t chunk_size) {
    return {
        floor_div(tile.x, chunk_size),
        floor_div(tile.y, chunk_size),
    };
}

TileCoord chunk_to_tile(const ChunkCoord& chunk, std::int32_t chunk_size) {
    return {
        chunk.x * chunk_size,
        chunk.y * chunk_size,
    };
}

std::optional<Side> side_from_string(const std::string& s) {
    if (s == "n") {
        return Side::North;
    }
    if (s == "e") {
        return Side::East;
    }
    if (s == "s") {
        return Side::South;
    }
    if (s == "w") {
        return Side::West;
    }
    return std::nullopt;
}

std::string side_to_string(Side side) {
    switch (side) {
    case Side::North:
        return "n";
    case Side::East:
        return "e";
    case Side::South:
        return "s";
    case Side::West:
        return "w";
    }
    return "n";
}

std::int64_t tile_key(std::int32_t x, std::int32_t y) {
    return (static_cast<std::int64_t>(x) << 32) | (static_cast<std::uint32_t>(y));
}

void tile_key_decode(std::int64_t key, std::int32_t& x, std::int32_t& y) {
    x = static_cast<std::int32_t>(key >> 32);
    y = static_cast<std::int32_t>(static_cast<std::uint32_t>(key));
}

const Level* World::level_or_null(std::int32_t z) const {
    const auto it = levels_.find(z);
    return it != levels_.end() ? &it->second : nullptr;
}

std::int32_t World::tile_id_at(const Level& lvl, const TileCoord& tile) const {
    const auto it = lvl.tiles.find(tile_key(tile.x, tile.y));
    return it != lvl.tiles.end() ? it->second : 0; // 未声明 = 空 (void) = 0
}

std::optional<World> World::load(const config::Document& doc) {
    World world;

    world.tile_size_ = static_cast<float>(doc.get_double("tile_size", 1.0));
    if (world.tile_size_ <= 0.0f) {
        app_log::error("world: invalid tile_size (must be > 0)");
        return std::nullopt;
    }
    world.chunk_size_ = static_cast<std::int32_t>(doc.get_int("chunk_size", 16));
    if (world.chunk_size_ <= 0) {
        app_log::error("world: invalid chunk_size (must be > 0)");
        return std::nullopt;
    }
    world.level_height_ = static_cast<float>(doc.get_double("level_height", 1.0));
    if (world.level_height_ <= 0.0f) {
        app_log::error("world: invalid level_height (must be > 0)");
        return std::nullopt;
    }
    world.vertical_scale_ = static_cast<float>(doc.get_double("vertical_scale", 1.0));
    if (world.vertical_scale_ <= 0.0f) {
        app_log::error("world: invalid vertical_scale (must be > 0)");
        return std::nullopt;
    }

    // tile 定义。
    const std::int64_t def_count = doc.size("tile_defs");
    if (def_count < 0) {
        app_log::error("world: missing or invalid 'tile_defs'");
        return std::nullopt;
    }
    for (std::int64_t i = 0; i < def_count; ++i) {
        const std::string base = "tile_defs." + std::to_string(i);
        const std::int32_t id = static_cast<std::int32_t>(doc.get_int(base + ".id", 0));
        if (id <= 0) {
            app_log::error("world: tile_defs[" + std::to_string(i) + "] has invalid id (must be > 0)");
            return std::nullopt;
        }
        if (world.tile_defs_.count(id) > 0) {
            app_log::error("world: duplicate tile id " + std::to_string(id));
            return std::nullopt;
        }
        TileDef def;
        def.id = id;
        def.name = doc.get_string(base + ".name", "");
        def.walkable = doc.get_bool(base + ".walkable", true);
        def.height = static_cast<float>(doc.get_double(base + ".height", 0.0));
        def.r = static_cast<float>(doc.get_double(base + ".color.0", 0.5));
        def.g = static_cast<float>(doc.get_double(base + ".color.1", 0.5));
        def.b = static_cast<float>(doc.get_double(base + ".color.2", 0.5));
        def.texture = doc.get_string(base + ".texture", "");
        world.tile_defs_.emplace(id, std::move(def));
    }

    // 层级 (按 z)。
    const std::int64_t level_count = doc.size("levels");
    if (level_count <= 0) {
        app_log::error("world: missing or empty 'levels'");
        return std::nullopt;
    }
    for (std::int64_t li = 0; li < level_count; ++li) {
        const std::string base = "levels." + std::to_string(li);
        const std::string id = doc.get_string(base + ".id", "");
        if (id.empty()) {
            app_log::error("world: levels[" + std::to_string(li) + "] missing 'id'");
            return std::nullopt;
        }
        const std::int32_t z = static_cast<std::int32_t>(doc.get_int(base + ".z", 0));
        if (world.levels_.count(z) > 0 || world.level_index_.count(id) > 0) {
            app_log::error("world: duplicate level z " + std::to_string(z) + " or id '" + id + "'");
            return std::nullopt;
        }

        Level lvl;
        lvl.id = id;
        lvl.width = static_cast<std::int32_t>(doc.get_int(base + ".width", 0));
        lvl.depth = static_cast<std::int32_t>(doc.get_int(base + ".depth", 0));

        // tiles (稀疏: 仅非空 tile)。
        const std::int64_t tile_count = doc.size(base + ".tiles");
        if (tile_count < 0) {
            app_log::error("world: level '" + id + "' has invalid 'tiles'");
            return std::nullopt;
        }
        lvl.tiles.reserve(static_cast<std::size_t>(tile_count));
        for (std::int64_t ti = 0; ti < tile_count; ++ti) {
            const std::string tbase = base + ".tiles." + std::to_string(ti);
            const std::int32_t x = static_cast<std::int32_t>(doc.get_int(tbase + ".x", 0));
            const std::int32_t y = static_cast<std::int32_t>(doc.get_int(tbase + ".y", 0));
            const std::int32_t tile_id = static_cast<std::int32_t>(doc.get_int(tbase + ".id", 0));
            if (world.tile_defs_.count(tile_id) == 0) {
                app_log::error("world: level '" + id + "' references unknown tile id " + std::to_string(tile_id));
                return std::nullopt;
            }
            const std::int64_t key = tile_key(x, y);
            if (lvl.tiles.count(key) > 0) {
                app_log::error("world: level '" + id + "' has duplicate tile at (" + std::to_string(x) + "," +
                               std::to_string(y) + ")");
                return std::nullopt;
            }
            lvl.tiles.emplace(key, tile_id);
            const bool interior = doc.get_bool(tbase + ".interior", false);
            if (interior) {
                lvl.interior_tiles.insert(key);
            }
        }

        // walls。
        const std::int64_t wall_count = doc.size(base + ".walls");
        if (wall_count < 0) {
            app_log::error("world: level '" + id + "' has invalid 'walls'");
            return std::nullopt;
        }
        for (std::int64_t wi = 0; wi < wall_count; ++wi) {
            const std::string wbase = base + ".walls." + std::to_string(wi);
            const std::int32_t x = static_cast<std::int32_t>(doc.get_int(wbase + ".x", 0));
            const std::int32_t y = static_cast<std::int32_t>(doc.get_int(wbase + ".y", 0));
            const std::optional<Side> side = side_from_string(doc.get_string(wbase + ".side", ""));
            if (!side.has_value()) {
                app_log::error("world: level '" + id + "' has invalid wall side at index " + std::to_string(wi));
                return std::nullopt;
            }
            const Wall wall{TileCoord{x, y}, *side};
            lvl.walls.push_back(wall);
            lvl.wall_set.insert(wall_key(x, y, *side));
        }

        world.level_index_.emplace(id, z);
        world.levels_.emplace(z, std::move(lvl));
    }

    return world;
}

const TileDef* World::tile_def(std::int32_t id) const {
    const auto it = tile_defs_.find(id);
    return it != tile_defs_.end() ? &it->second : nullptr;
}

std::int32_t World::level_count() const {
    return static_cast<std::int32_t>(levels_.size());
}

const Level* World::level(std::int32_t z) const {
    return level_or_null(z);
}

const Level* World::level(const std::string& id) const {
    const auto it = level_index_.find(id);
    if (it == level_index_.end()) {
        return nullptr;
    }
    return level_or_null(it->second);
}

float World::level_height() const {
    return level_height_;
}

float World::vertical_scale() const {
    return vertical_scale_;
}

std::vector<std::int32_t> World::level_zs() const {
    std::vector<std::int32_t> zs;
    zs.reserve(levels_.size());
    for (const auto& entry : levels_) {
        zs.push_back(entry.first);
    }
    return zs;
}

bool World::is_walkable(std::int32_t z, const TileCoord& tile) const {
    const TileDef* def = tile_def_at(z, tile);
    return def != nullptr && def->walkable;
}

bool World::has_wall(std::int32_t z, const TileCoord& tile, Side side) const {
    const Level* lvl = level_or_null(z);
    if (lvl == nullptr) {
        return false;
    }
    // 对称查询: 本格该侧, 或相邻格对侧, 任一命中即为有墙。
    if (lvl->wall_set.count(wall_key(tile.x, tile.y, side)) > 0) {
        return true;
    }
    const TileCoord adj = neighbor(tile, side);
    return lvl->wall_set.count(wall_key(adj.x, adj.y, opposite(side))) > 0;
}

const TileDef* World::tile_def_at(std::int32_t z, const TileCoord& tile) const {
    const Level* lvl = level_or_null(z);
    if (lvl == nullptr) {
        return nullptr;
    }
    const std::int32_t id = tile_id_at(*lvl, tile);
    if (id == 0) {
        return nullptr;
    }
    return tile_def(id);
}

bool World::is_interior(std::int32_t z, const TileCoord& tile) const {
    const Level* lvl = level_or_null(z);
    if (lvl == nullptr) {
        return false;
    }
    return lvl->interior_tiles.count(tile_key(tile.x, tile.y)) > 0;
}

float World::tile_size() const {
    return tile_size_;
}

std::int32_t World::chunk_size() const {
    return chunk_size_;
}

std::int32_t World::width(std::int32_t z) const {
    const Level* lvl = level_or_null(z);
    return lvl != nullptr ? lvl->width : 0;
}

std::int32_t World::depth(std::int32_t z) const {
    const Level* lvl = level_or_null(z);
    return lvl != nullptr ? lvl->depth : 0;
}
} // namespace world
