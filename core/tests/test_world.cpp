#include "world/world.h"

#include <catch2/catch_test_macros.hpp>
#include <string>

namespace {
std::string config_path(const char* name) {
    return std::string(PROJECT_PHOENIX_CONFIG_DIR) + "/" + name;
}

world::World load_world(const char* name) {
    const auto doc = config::load_file(config_path(name));
    REQUIRE(doc.has_value());
    auto world = world::World::load(*doc);
    REQUIRE(world.has_value());
    return *world;
}
} // namespace

TEST_CASE("world coordinate conversion") {
    // world → tile (含负数向下取整)。
    REQUIRE(world::world_to_tile(0.0f, 0.0f, 1.0f).x == 0);
    REQUIRE(world::world_to_tile(0.0f, 0.0f, 1.0f).y == 0);
    REQUIRE(world::world_to_tile(0.9f, 1.9f, 1.0f).x == 0);
    REQUIRE(world::world_to_tile(0.9f, 1.9f, 1.0f).y == 1);
    REQUIRE(world::world_to_tile(-0.1f, -0.1f, 1.0f).x == -1);
    REQUIRE(world::world_to_tile(-0.1f, -0.1f, 1.0f).y == -1);
    REQUIRE(world::world_to_tile(-1.0f, -1.0f, 1.0f).x == -1);
    REQUIRE(world::world_to_tile(-1.0f, -1.0f, 1.0f).y == -1);

    // tile → world (中心点)。
    float wx = 0.0f;
    float wy = 0.0f;
    world::tile_to_world({2, 3}, 1.0f, wx, wy);
    REQUIRE(wx == 2.5f);
    REQUIRE(wy == 3.5f);
}

TEST_CASE("world chunk conversion") {
    REQUIRE(world::tile_to_chunk({0, 0}, 2).x == 0);
    REQUIRE(world::tile_to_chunk({1, 1}, 2).y == 0);
    REQUIRE(world::tile_to_chunk({2, 2}, 2).x == 1);
    REQUIRE(world::tile_to_chunk({-1, -1}, 2).x == -1);
    REQUIRE(world::tile_to_chunk({-1, -1}, 2).y == -1);

    const auto origin = world::chunk_to_tile({1, 1}, 2);
    REQUIRE(origin.x == 2);
    REQUIRE(origin.y == 2);
}

TEST_CASE("world load tile defs") {
    const world::World world = load_world("test_world.json");

    const world::TileDef* grass = world.tile_def(1);
    REQUIRE(grass != nullptr);
    REQUIRE(grass->walkable == true);
    REQUIRE(grass->height == 0.0f);

    const world::TileDef* stone = world.tile_def(2);
    REQUIRE(stone != nullptr);
    REQUIRE(stone->walkable == true);

    const world::TileDef* wall = world.tile_def(3);
    REQUIRE(wall != nullptr);
    REQUIRE(wall->walkable == false);
    REQUIRE(wall->height == 1.0f);

    REQUIRE(world.tile_def(999) == nullptr);
}

TEST_CASE("world load levels") {
    const world::World world = load_world("test_world.json");

    REQUIRE(world.level_count() == 2);
    REQUIRE(world.level_height() == 1.0f);
    REQUIRE(world.width(0) == 4);
    REQUIRE(world.depth(0) == 4);

    REQUIRE(world.level(0) != nullptr);
    REQUIRE(world.level(0)->id == "ground");
    REQUIRE(world.level(1) != nullptr);
    REQUIRE(world.level(1)->id == "upper");
    REQUIRE(world.level("ground") == world.level(0));
    REQUIRE(world.level("upper") == world.level(1));
    REQUIRE(world.level("missing") == nullptr);
    REQUIRE(world.level(2) == nullptr);

    const std::vector<std::int32_t> zs = world.level_zs();
    REQUIRE(zs.size() == 2);
    REQUIRE(zs[0] == 0);
    REQUIRE(zs[1] == 1);
}

TEST_CASE("world walkability and tile query") {
    const world::World world = load_world("test_world.json");

    // (0,0) = stone 可走; (1,1) = wall 不可走; (2,2) = grass 可走。
    REQUIRE(world.is_walkable(0, {0, 0}) == true);
    REQUIRE(world.is_walkable(0, {1, 1}) == false);
    REQUIRE(world.is_walkable(0, {2, 2}) == true);

    REQUIRE(world.tile_def_at(0, {0, 0}) == world.tile_def(2));
    REQUIRE(world.tile_def_at(0, {1, 1}) == world.tile_def(3));
    REQUIRE(world.tile_def_at(0, {2, 2}) == world.tile_def(1));

    // 越界查询返回安全默认值, 不崩溃。
    REQUIRE(world.is_walkable(0, {-1, 0}) == false);
    REQUIRE(world.is_walkable(0, {4, 0}) == false);
    REQUIRE(world.is_walkable(0, {0, 4}) == false);
    REQUIRE(world.tile_def_at(0, {9, 9}) == nullptr);
    REQUIRE(world.is_walkable(9, {0, 0}) == false);
}

TEST_CASE("world wall query with symmetric lookup") {
    const world::World world = load_world("test_world.json");

    // 直接命中: (2,1) 的东边 (e) 有墙。
    REQUIRE(world.has_wall(0, {2, 1}, world::Side::East) == true);
    // 对称查询: (3,1) 的西边 (w) 与 (2,1) 的东边是同一面墙。
    REQUIRE(world.has_wall(0, {3, 1}, world::Side::West) == true);
    // 无墙侧。
    REQUIRE(world.has_wall(0, {2, 1}, world::Side::North) == false);
    REQUIRE(world.has_wall(0, {0, 0}, world::Side::East) == false);
}

TEST_CASE("world vertical scale") {
    // test_world.json 声明 vertical_scale = 3.0。
    const world::World world = load_world("test_world.json");
    REQUIRE(world.vertical_scale() == 3.0f);

    // 未声明字段 → 默认 1.0。
    const world::World sparse = load_world("test_world_sparse.json");
    REQUIRE(sparse.vertical_scale() == 1.0f);
}

TEST_CASE("world invalid vertical scale fails load") {
    const auto doc = config::load_file(config_path("test_world_bad_scale.json"));
    REQUIRE(doc.has_value());
    REQUIRE_FALSE(world::World::load(*doc).has_value());
}

TEST_CASE("world interior flags") {
    const world::World world = load_world("test_world.json");

    // (0,0) 与 (1,1) 在 z=0 声明了 interior。
    REQUIRE(world.is_interior(0, {0, 0}) == true);
    REQUIRE(world.is_interior(0, {1, 1}) == true);
    // 未标记 / 上层 / 越界 / 层级不存在 → false。
    REQUIRE(world.is_interior(0, {2, 2}) == false);
    REQUIRE(world.is_interior(1, {0, 0}) == false);
    REQUIRE(world.is_interior(0, {9, 9}) == false);
    REQUIRE(world.is_interior(5, {0, 0}) == false);
    // interior 与 walkable 无关: (1,1) 是 wall 不可走但 interior 为 true。
    REQUIRE(world.is_walkable(0, {1, 1}) == false);
    REQUIRE(world.is_interior(0, {1, 1}) == true);
}

TEST_CASE("world invalid JSON returns nullopt") {
    const auto doc = config::load_file(config_path("test_invalid.json"));
    REQUIRE_FALSE(doc.has_value());
}

TEST_CASE("world unknown tile id in map fails load") {
    const auto doc = config::load_file(config_path("test_world_unknown_tile.json"));
    REQUIRE(doc.has_value());
    REQUIRE_FALSE(world::World::load(*doc).has_value());
}

TEST_CASE("world duplicate tile id fails load") {
    const auto doc = config::load_file(config_path("test_world_duplicate_id.json"));
    REQUIRE(doc.has_value());
    REQUIRE_FALSE(world::World::load(*doc).has_value());
}

TEST_CASE("world sparse tiles treat missing tiles as empty") {
    const world::World world = load_world("test_world_sparse.json");

    // 声明内容: (0,0) floor, (2,2) wall。
    REQUIRE(world.is_walkable(0, {0, 0}) == true);
    REQUIRE(world.is_walkable(0, {2, 2}) == false);
    REQUIRE(world.tile_def_at(0, {0, 0}) == world.tile_def(1));
    REQUIRE(world.tile_def_at(0, {2, 2}) == world.tile_def(2));

    // 未声明 = 空 (void): 不可走、无 def。
    REQUIRE(world.is_walkable(0, {1, 1}) == false);
    REQUIRE(world.tile_def_at(0, {1, 1}) == nullptr);
    REQUIRE(world.tile_def_at(0, {3, 3}) == nullptr);

    // 越界仍安全返回空。
    REQUIRE(world.is_walkable(0, {4, 0}) == false);
    REQUIRE(world.tile_def_at(0, {9, 9}) == nullptr);
}
