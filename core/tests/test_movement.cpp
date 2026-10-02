#include "movement/movement.h"

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

TEST_CASE("movement straight move") {
    const world::World world = load_world("test_world.json");

    ecs::Position pos{0.5f, 0.5f, 0.0f}; // tile (0,0) stone
    ecs::Mover mover;
    mover.speed = 1.0f;
    mover.z = 0;

    const movement::MoveResult result = movement::step(world, pos, mover, 1.0f, 0.0f, 1.0f);

    REQUIRE(result.moved == true);
    REQUIRE(pos.x == 1.5f); // 前进 1 单位
    REQUIRE(pos.y == 0.5f);
    REQUIRE(pos.z == 0.0f);
}

TEST_CASE("movement blocked by non-walkable tile") {
    const world::World world = load_world("test_world.json");

    ecs::Position pos{0.5f, 1.5f, 0.0f}; // tile (0,1) grass, +x 是 (1,1) wall
    ecs::Mover mover;
    mover.speed = 1.0f;

    const movement::MoveResult result = movement::step(world, pos, mover, 1.0f, 0.0f, 1.0f);

    REQUIRE(result.moved == false);
    REQUIRE(pos.x == 0.5f); // 未移动
}

TEST_CASE("movement blocked by edge wall") {
    const world::World world = load_world("test_world.json");

    ecs::Position pos{2.5f, 1.5f, 0.0f}; // tile (2,1), 东边有墙 {2,1} side e
    ecs::Mover mover;
    mover.speed = 1.0f;

    const movement::MoveResult result = movement::step(world, pos, mover, 1.0f, 0.0f, 1.0f);

    REQUIRE(result.moved == false);
    REQUIRE(pos.x == 2.5f); // 被东边墙阻挡
}

TEST_CASE("movement slides along wall (axis separation)") {
    const world::World world = load_world("test_world.json");

    ecs::Position pos{2.5f, 1.5f, 0.0f}; // tile (2,1), 东边有墙
    ecs::Mover mover;
    mover.speed = 1.0f;

    // 斜向东北: X 被东边墙挡, Y 应仍能向北移动。
    const movement::MoveResult result = movement::step(world, pos, mover, 1.0f, -1.0f, 1.0f);

    REQUIRE(result.moved == true);
    REQUIRE(pos.x == 2.5f); // X 未动
    REQUIRE(pos.y < 1.5f);  // Y 向北移动
}

TEST_CASE("movement snaps z to tile height") {
    const world::World world = load_world("test_movement.json");

    ecs::Position pos{1.5f, 1.5f, 0.0f}; // tile (1,1) grass, -y 是 (1,0) raised (height 0.5)
    ecs::Mover mover;
    mover.speed = 1.0f;
    mover.z = 0;

    const movement::MoveResult result = movement::step(world, pos, mover, 0.0f, -1.0f, 1.0f);

    REQUIRE(result.moved == true);
    REQUIRE(pos.y == 0.5f);
    REQUIRE(pos.z == 0.5f); // 层级 0 * 层高 + raised height 0.5
}

TEST_CASE("movement door gap passable and wall blocks only at seam") {
    // test_visibility.json: 房屋 3x3 (x,y ∈ [2,4]), 南墙 y=5 在 x=3 有 1 格门洞, 北墙 y=2。
    const world::World world = load_world("test_visibility.json");

    // ① 门洞列 x=3: 从南侧一路向北, 穿过南墙门洞 (y=5), 最终被北墙挡在接缝 y=2 (内侧 tile (3,2) 可进入)。
    {
        ecs::Position pos{3.5f, 6.5f, 0.0f};
        ecs::Mover mover;
        mover.speed = 0.1f;
        mover.z = 0;
        for (int i = 0; i < 100; ++i) {
            movement::step(world, pos, mover, 0.0f, -1.0f, 1.0f);
        }
        REQUIRE(pos.y >= 2.0f);
        REQUIRE(pos.y < 2.1f);
    }

    // ② 墙段列 x=2 (南墙 (2,4) side s): 从南侧向北, 停在接缝 y=5 (墙外一格 tile (2,5) 可进入)。
    {
        ecs::Position pos{2.5f, 6.5f, 0.0f};
        ecs::Mover mover;
        mover.speed = 0.1f;
        mover.z = 0;
        for (int i = 0; i < 100; ++i) {
            movement::step(world, pos, mover, 0.0f, -1.0f, 1.0f);
        }
        REQUIRE(pos.y >= 5.0f);
        REQUIRE(pos.y < 5.1f);
    }

    // ③ 同一墙段从内侧 (tile (2,4)) 向南: 停在接缝 y=5 (墙内一格 tile (2,4) 可进入且可贴墙移动)。
    {
        ecs::Position pos{2.5f, 4.5f, 0.0f};
        ecs::Mover mover;
        mover.speed = 0.1f;
        mover.z = 0;
        for (int i = 0; i < 100; ++i) {
            movement::step(world, pos, mover, 0.0f, 1.0f, 1.0f);
        }
        REQUIRE(pos.y >= 4.9f);
        REQUIRE(pos.y < 5.0f);
    }

    // ④ 墙内一圈贴墙移动不受阻: tile (2,4) 内朝墙方向可移动直至接缝 (从 4.5 起步至少能走到 4.6+)。
    {
        ecs::Position pos{2.5f, 4.5f, 0.0f};
        ecs::Mover mover;
        mover.speed = 0.1f;
        mover.z = 0;
        const movement::MoveResult result = movement::step(world, pos, mover, 0.0f, 1.0f, 1.0f);
        REQUIRE(result.moved == true);
        REQUIRE(pos.y > 4.5f);
    }
}
