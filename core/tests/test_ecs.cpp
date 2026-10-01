#include "ecs/ecs.h"

#include <catch2/catch_test_macros.hpp>
#include <string>

namespace {
struct Position {
    float x;
    float y;
};

struct Velocity {
    float vx;
    float vy;
};
} // namespace

TEST_CASE("create and destroy entities") {
    ecs::Registry registry;

    const ecs::Entity entity = registry.create();
    REQUIRE(entity != ecs::null);
    REQUIRE(registry.alive(entity));

    registry.destroy(entity);
    REQUIRE_FALSE(registry.alive(entity));
}

TEST_CASE("attach get and has components") {
    ecs::Registry registry;
    const ecs::Entity entity = registry.create();

    registry.attach<Position>(entity, 1.0f, 2.0f);
    REQUIRE(registry.has<Position>(entity));
    REQUIRE(registry.get<Position>(entity).x == 1.0f);
    REQUIRE(registry.get<Position>(entity).y == 2.0f);

    registry.attach_or_replace<Position>(entity, 3.0f, 4.0f);
    REQUIRE(registry.get<Position>(entity).x == 3.0f);
    REQUIRE(registry.get<Position>(entity).y == 4.0f);

    registry.detach<Position>(entity);
    REQUIRE_FALSE(registry.has<Position>(entity));
}

TEST_CASE("view iterates the correct entity count") {
    ecs::Registry registry;
    const ecs::Entity a = registry.create();
    const ecs::Entity b = registry.create();
    const ecs::Entity c = registry.create();

    registry.attach<Position>(a, 0.0f, 0.0f);
    registry.attach<Position>(b, 0.0f, 0.0f);
    registry.attach<Position>(c, 0.0f, 0.0f);
    registry.attach<Velocity>(a, 0.0f, 0.0f);
    registry.attach<Velocity>(b, 0.0f, 0.0f);

    int position_count = 0;
    for (const ecs::Entity entity : registry.view<Position>()) {
        (void)entity;
        ++position_count;
    }
    REQUIRE(position_count == 3);

    int both_count = 0;
    for (const ecs::Entity entity : registry.view<Position, Velocity>()) {
        (void)entity;
        ++both_count;
    }
    REQUIRE(both_count == 2);
}

TEST_CASE("destroyed entities have their components cleaned up") {
    ecs::Registry registry;
    const ecs::Entity a = registry.create();
    const ecs::Entity b = registry.create();

    registry.attach<Position>(a, 0.0f, 0.0f);
    registry.attach<Position>(b, 0.0f, 0.0f);

    registry.destroy(a);

    int count = 0;
    for (const ecs::Entity entity : registry.view<Position>()) {
        (void)entity;
        ++count;
    }
    REQUIRE(count == 1);
}

TEST_CASE("system manager runs systems in registration order with dt") {
    ecs::Registry registry;
    ecs::SystemManager systems;

    std::string order;
    float received_dt = 0.0f;
    systems.add([&](ecs::Registry&, float dt) {
        order += 'a';
        received_dt = dt;
    });
    systems.add([&](ecs::Registry&, float) { order += 'b'; });
    systems.add([&](ecs::Registry&, float) { order += 'c'; });

    systems.update(registry, 0.25f);

    REQUIRE(order == "abc");
    REQUIRE(received_dt == 0.25f);
}

TEST_CASE("system can mutate components through the registry") {
    ecs::Registry registry;
    ecs::SystemManager systems;

    const ecs::Entity entity = registry.create();
    registry.attach<Position>(entity, 1.0f, 1.0f);

    systems.add([](ecs::Registry& reg, float) {
        for (const ecs::Entity e : reg.view<Position>()) {
            reg.get<Position>(e).x += 1.0f;
        }
    });

    systems.update(registry, 0.0f);

    REQUIRE(registry.get<Position>(entity).x == 2.0f);
}
