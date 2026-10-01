#include "camera/camera.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <string>

namespace {
camera::Params isometric_params(float distance, float fovy = 45.0f) {
    camera::Params p;
    p.distance = distance;
    p.fovy = fovy;
    return p;
}
} // namespace

TEST_CASE("create camera returns id and rejects duplicate names") {
    camera::Manager manager;

    const camera::Id id = manager.create("main", camera::Type::Isometric, isometric_params(15.0f));
    REQUIRE(id != camera::kInvalidId);
    REQUIRE(manager.has(id));
    REQUIRE(manager.has("main"));

    REQUIRE(manager.create("main", camera::Type::Isometric, isometric_params(10.0f)) == camera::kInvalidId);
}

TEST_CASE("cameras are addressable by both name and id") {
    camera::Manager manager;
    const camera::Id id = manager.create("main", camera::Type::Isometric, isometric_params(15.0f));

    REQUIRE(manager.find("main") == id);
    REQUIRE(manager.find("missing") == camera::kInvalidId);
    REQUIRE(manager.names() == std::vector<std::string>{"main"});
}

TEST_CASE("active camera switches by id and name") {
    camera::Manager manager;
    const camera::Id a = manager.create("a", camera::Type::Isometric, isometric_params(15.0f));
    const camera::Id b = manager.create("b", camera::Type::Isometric, isometric_params(25.0f));

    REQUIRE(manager.active() == camera::kInvalidId);
    REQUIRE(manager.active_name().empty());

    REQUIRE(manager.set_active(a));
    REQUIRE(manager.active() == a);
    REQUIRE(manager.active_name() == "a");

    REQUIRE(manager.set_active("b"));
    REQUIRE(manager.active() == b);
    REQUIRE(manager.active_name() == "b");
}

TEST_CASE("set_param changes fov on the active camera") {
    camera::Manager manager;
    manager.create("main", camera::Type::Isometric, isometric_params(15.0f, 45.0f));
    manager.set_active("main");

    REQUIRE(manager.set_param_active("fov", 60.0));
    REQUIRE(manager.active_params()->fovy == 60.0f);

    // fovy 别名等价。
    REQUIRE(manager.set_param_active("fovy", 30.0));
    REQUIRE(manager.active_params()->fovy == 30.0f);
}

TEST_CASE("set_param distance repositions the isometric camera") {
    camera::Manager manager;
    manager.create("main", camera::Type::Isometric, isometric_params(15.0f));
    manager.set_active("main");

    REQUIRE(manager.set_param_active("distance", 20.0));

    const camera::Params* p = manager.active_params();
    REQUIRE(p->distance == 20.0f);
    REQUIRE(p->pos[0] == Catch::Approx(camera::kIsoDirX * 20.0f));
    REQUIRE(p->pos[1] == Catch::Approx(camera::kIsoDirY * 20.0f));
    REQUIRE(p->pos[2] == Catch::Approx(camera::kIsoDirZ * 20.0f));
}

TEST_CASE("set_param rejects unknown keys") {
    camera::Manager manager;
    manager.create("main", camera::Type::Isometric, isometric_params(15.0f));
    manager.set_active("main");

    REQUIRE_FALSE(manager.set_param_active("nope", 1.0));
}

TEST_CASE("destroy removes camera and clears active when destroyed") {
    camera::Manager manager;
    const camera::Id a = manager.create("a", camera::Type::Isometric, isometric_params(15.0f));
    manager.create("b", camera::Type::Isometric, isometric_params(25.0f));
    manager.set_active(a);

    REQUIRE(manager.destroy("a"));
    REQUIRE_FALSE(manager.has("a"));
    REQUIRE_FALSE(manager.has(a));
    REQUIRE(manager.active() == camera::kInvalidId);
    REQUIRE(manager.active_name().empty());
    REQUIRE(manager.names() == std::vector<std::string>{"b"});
}

TEST_CASE("apply_isometric derives position from target and distance") {
    camera::Params p;
    p.target[0] = 1.0f;
    p.target[1] = 2.0f;
    p.target[2] = 3.0f;
    p.distance = 10.0f;

    camera::apply_isometric(p);

    REQUIRE(p.pos[0] == Catch::Approx(1.0f + camera::kIsoDirX * 10.0f));
    REQUIRE(p.pos[1] == Catch::Approx(2.0f + camera::kIsoDirY * 10.0f));
    REQUIRE(p.pos[2] == Catch::Approx(3.0f + camera::kIsoDirZ * 10.0f));
}
