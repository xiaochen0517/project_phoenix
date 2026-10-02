#include "visibility/visibility.h"

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

// 玩家世界坐标 = tile 中心 (tile.x + 0.5, tile.y + 0.5)。
visibility::Culler make_culler(const world::World& world, float x, float y, std::int32_t z) {
    visibility::Culler culler;
    culler.update(world, {x, y, z});
    return culler;
}
} // namespace

TEST_CASE("visibility interior player dissolves upper and cuts wall lines") {
    const world::World world = load_world("test_visibility.json");

    // 屋内: tile (3,3) 为 interior (门洞列)。
    const visibility::Culler culler = make_culler(world, 3.5f, 3.5f, 0);

    // 规则 A: 屋内 → 溶解所有更高层。
    REQUIRE(culler.upper_dissolved() == true);

    // 规则 B: 南墙线 (y=5) 全部 S 墙隐藏 (含门洞两侧)。
    REQUIRE(culler.is_wall_hidden({{2, 4}, world::Side::South}) == true);
    REQUIRE(culler.is_wall_hidden({{4, 4}, world::Side::South}) == true);
    // 东墙线 (x=5) 全部 E 墙隐藏。
    REQUIRE(culler.is_wall_hidden({{4, 2}, world::Side::East}) == true);
    REQUIRE(culler.is_wall_hidden({{4, 3}, world::Side::East}) == true);
    REQUIRE(culler.is_wall_hidden({{4, 4}, world::Side::East}) == true);
    // 北/西墙不隐藏 (位于玩家屏幕后/左侧, 不遮挡视线)。
    REQUIRE(culler.is_wall_hidden({{2, 2}, world::Side::North}) == false);
    REQUIRE(culler.is_wall_hidden({{3, 2}, world::Side::North}) == false);
    REQUIRE(culler.is_wall_hidden({{4, 2}, world::Side::North}) == false);
    REQUIRE(culler.is_wall_hidden({{2, 2}, world::Side::West}) == false);
    REQUIRE(culler.is_wall_hidden({{2, 3}, world::Side::West}) == false);
    REQUIRE(culler.is_wall_hidden({{2, 4}, world::Side::West}) == false);
}

TEST_CASE("visibility player northwest of building hides all upper and all walls") {
    const world::World world = load_world("test_visibility.json");

    // 玩家在建筑西北 (0.5, 0.5), 建筑全部 interior tile 位于玩家东南象限。
    const visibility::Culler culler = make_culler(world, 0.5f, 0.5f, 0);

    // 规则 A: 直接隐藏所有更高层内容。
    REQUIRE(culler.upper_dissolved() == true);
    // 规则 B: 当前层级的所有墙设置为透明 (东西南北全隐藏)。
    REQUIRE(culler.is_wall_hidden({{2, 4}, world::Side::South}) == true);
    REQUIRE(culler.is_wall_hidden({{4, 4}, world::Side::South}) == true);
    REQUIRE(culler.is_wall_hidden({{2, 2}, world::Side::North}) == true);
    REQUIRE(culler.is_wall_hidden({{2, 2}, world::Side::West}) == true);
    REQUIRE(culler.is_wall_hidden({{2, 3}, world::Side::West}) == true);
    REQUIRE(culler.is_wall_hidden({{4, 2}, world::Side::East}) == true);
}

TEST_CASE("visibility player far northwest of building still hides (no radius)") {
    const world::World world = load_world("test_visibility.json");

    // 玩家在建筑西北远处 (-6.5, -6.5): 不设距离阈值, 仍直接隐藏。
    const visibility::Culler culler = make_culler(world, -6.5f, -6.5f, 0);

    REQUIRE(culler.upper_dissolved() == true);
    REQUIRE(culler.is_wall_hidden({{2, 4}, world::Side::South}) == true);
    REQUIRE(culler.is_wall_hidden({{2, 2}, world::Side::North}) == true);
}

TEST_CASE("visibility player southeast of building performs no dissolve or cutaway") {
    const world::World world = load_world("test_visibility.json");

    // 玩家在建筑东南 (6.5, 6.5), 建筑位于玩家西北 (屏幕后方) → 不进行任何透明/溶解操作。
    const visibility::Culler culler = make_culler(world, 6.5f, 6.5f, 0);

    REQUIRE(culler.upper_dissolved() == false);
    REQUIRE(culler.is_wall_hidden({{2, 4}, world::Side::South}) == false);
    REQUIRE(culler.is_wall_hidden({{4, 4}, world::Side::South}) == false);
    REQUIRE(culler.is_wall_hidden({{2, 2}, world::Side::North}) == false);
    REQUIRE(culler.is_wall_hidden({{2, 2}, world::Side::West}) == false);
    REQUIRE(culler.is_wall_hidden({{4, 2}, world::Side::East}) == false);
}

TEST_CASE("visibility wall line scan covers door column") {
    const world::World world = load_world("test_visibility.json");

    // 屋内且玩家列 x=3 正对 1 格门洞: 南墙线仍应通过相邻列 (x=2) 命中; 东墙线照常命中。
    const visibility::Culler culler = make_culler(world, 3.5f, 3.5f, 0);

    REQUIRE(culler.upper_dissolved() == true);
    REQUIRE(culler.is_wall_hidden({{2, 4}, world::Side::South}) == true);
    REQUIRE(culler.is_wall_hidden({{4, 4}, world::Side::South}) == true);
    REQUIRE(culler.is_wall_hidden({{4, 2}, world::Side::East}) == true);
    REQUIRE(culler.is_wall_hidden({{4, 4}, world::Side::East}) == true);
    // 北墙不隐藏 (屋内只裁剪南/东墙)。
    REQUIRE(culler.is_wall_hidden({{2, 2}, world::Side::North}) == false);
    REQUIRE(culler.is_wall_hidden({{4, 2}, world::Side::North}) == false);
}

TEST_CASE("visibility content above without interior flag not treated as inside") {
    const world::World world = load_world("test_visibility.json");

    // tile (6,6) 上方有孤立 roof (z=1), 但 ground (6,6) 非 interior →
    // 不按屋内处理; 且玩家在建筑东南 (interior tile 均在西北象限) → 不触发任何溶解/透明。
    const visibility::Culler culler = make_culler(world, 6.5f, 6.5f, 0);

    REQUIRE(culler.upper_dissolved() == false);
    REQUIRE(culler.is_wall_hidden({{2, 4}, world::Side::South}) == false);
    REQUIRE(culler.is_wall_hidden({{2, 2}, world::Side::North}) == false);
}

TEST_CASE("visibility walls below player level never hidden") {
    const world::World world = load_world("test_visibility.json");

    // 玩家在 z=1 (屋顶): z=1 无 interior tile → 不触发任何规则。
    const visibility::Culler culler = make_culler(world, 3.5f, 3.5f, 1);

    REQUIRE(culler.upper_dissolved() == false);
    // z=0 的墙 (低于玩家层) 与 z=1 的墙均不隐藏。
    REQUIRE(culler.is_wall_hidden({{2, 4}, world::Side::South}) == false);
    REQUIRE(culler.is_wall_hidden({{4, 2}, world::Side::East}) == false);
}
