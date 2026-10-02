#include "world/world.h"

#include <catch2/catch_test_macros.hpp>
#include <string>

namespace {
std::string config_path(const char* name) {
    return std::string(PROJECT_PHOENIX_CONFIG_DIR) + "/" + name;
}

world::World load_test_world() {
    const auto doc = config::load_file(config_path("test_world.json"));
    REQUIRE(doc.has_value());
    auto world = world::World::load(*doc);
    REQUIRE(world.has_value());
    return *world;
}
} // namespace

TEST_CASE("world coordinate conversion") {
    // world → tile (含负数向下取整)。
    REQUIRE(world::world_to_tile(0.0f, 0.0f, 1.0f).x == 0);
    REQUIRE(world::world_to_tile(0.0f, 0.0f, 1.0f).z == 0);
    REQUIRE(world::world_to_tile(0.9f, 1.9f, 1.0f).x == 0);
    REQUIRE(world::world_to_tile(0.9f, 1.9f, 1.0f).z == 1);
    REQUIRE(world::world_to_tile(-0.1f, -0.1f, 1.0f).x == -1);
    REQUIRE(world::world_to_tile(-0.1f, -0.1f, 1.0f).z == -1);
    REQUIRE(world::world_to_tile(-1.0f, -1.0f, 1.0f).x == -1);
    REQUIRE(world::world_to_tile(-1.0f, -1.0f, 1.0f).z == -1);

    // tile → world (中心点)。
    float wx = 0.0f;
    float wz = 0.0f;
    world::tile_to_world({2, 3}, 1.0f, wx, wz);
    REQUIRE(wx == 2.5f);
    REQUIRE(wz == 3.5f);
}

TEST_CASE("world chunk conversion") {
    REQUIRE(world::tile_to_chunk({0, 0}, 2).x == 0);
    REQUIRE(world::tile_to_chunk({1, 1}, 2).x == 0);
    REQUIRE(world::tile_to_chunk({2, 2}, 2).x == 1);
    REQUIRE(world::tile_to_chunk({-1, -1}, 2).x == -1);
    REQUIRE(world::tile_to_chunk({-1, -1}, 2).z == -1);

    const auto origin = world::chunk_to_tile({1, 1}, 2);
    REQUIRE(origin.x == 2);
    REQUIRE(origin.z == 2);
}

TEST_CASE("world load tile defs") {
    const world::World world = load_test_world();

    const world::TileDef* grass = world.tile_def(1);
    REQUIRE(grass != nullptr);
    REQUIRE(grass->walkable == true);
    REQUIRE(grass->height == 0.0f);

    const world::TileDef* wall = world.tile_def(3);
    REQUIRE(wall != nullptr);
    REQUIRE(wall->walkable == false);
    REQUIRE(wall->height == 1.0f);

    const world::TileDef* stairs_up = world.tile_def(4);
    REQUIRE(stairs_up != nullptr);
    REQUIRE(stairs_up->stairs == world::Stairs::Up);

    const world::TileDef* stairs_down = world.tile_def(5);
    REQUIRE(stairs_down != nullptr);
    REQUIRE(stairs_down->stairs == world::Stairs::Down);

    REQUIRE(world.tile_def(999) == nullptr);
}

TEST_CASE("world load levels") {
    const world::World world = load_test_world();

    REQUIRE(world.level_count() == 2);
    REQUIRE(world.width(0) == 4);
    REQUIRE(world.depth(0) == 4);
    REQUIRE(world.width(1) == 4);
    REQUIRE(world.depth(1) == 4);

    REQUIRE(world.level(0) != nullptr);
    REQUIRE(world.level(0)->id == "ground");
    REQUIRE(world.level(1) != nullptr);
    REQUIRE(world.level(1)->id == "upper");
    REQUIRE(world.level("ground") == world.level(0));
    REQUIRE(world.level("upper") == world.level(1));
    REQUIRE(world.level("missing") == nullptr);
    REQUIRE(world.level(2) == nullptr);
}

TEST_CASE("world walkability and tile query") {
    const world::World world = load_test_world();

    // (0,0) = stone (id 2) 可走; (1,1) = wall (id 3) 不可走; (2,2) = stairs_up (id 4) 可走。
    REQUIRE(world.is_walkable(0, {0, 0}) == true);
    REQUIRE(world.is_walkable(0, {1, 1}) == false);
    REQUIRE(world.is_walkable(0, {2, 2}) == true);

    REQUIRE(world.tile_def_at(0, {0, 0}) == world.tile_def(2));
    REQUIRE(world.tile_def_at(0, {1, 1}) == world.tile_def(3));
    REQUIRE(world.tile_def_at(0, {2, 2}) == world.tile_def(4));

    // 越界查询返回安全默认值, 不崩溃。
    REQUIRE(world.is_walkable(0, {-1, 0}) == false);
    REQUIRE(world.is_walkable(0, {4, 0}) == false);
    REQUIRE(world.is_walkable(0, {0, 4}) == false);
    REQUIRE(world.tile_def_at(0, {9, 9}) == nullptr);
    REQUIRE(world.is_walkable(9, {0, 0}) == false);
}

TEST_CASE("world wall query with symmetric lookup") {
    const world::World world = load_test_world();

    // 直接命中: (2,1) 的东边 (e) 有墙。
    REQUIRE(world.has_wall(0, {2, 1}, world::Side::East) == true);
    // 对称查询: (3,1) 的西边 (w) 与 (2,1) 的东边是同一面墙。
    REQUIRE(world.has_wall(0, {3, 1}, world::Side::West) == true);
    // 无墙侧。
    REQUIRE(world.has_wall(0, {2, 1}, world::Side::North) == false);
    REQUIRE(world.has_wall(0, {0, 0}, world::Side::East) == false);
}

TEST_CASE("world stairs query") {
    const world::World world = load_test_world();

    REQUIRE(world.stairs_at(0, {2, 2}) == world::Stairs::Up);
    REQUIRE(world.stairs_at(1, {2, 2}) == world::Stairs::Down);
    REQUIRE(world.stairs_at(0, {0, 0}) == world::Stairs::None);
    REQUIRE(world.stairs_at(0, {9, 9}) == world::Stairs::None);
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
