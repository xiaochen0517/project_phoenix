#include "camera/camera.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <string>
#include <vector>

namespace {
camera::Params isometric_params(float distance, float fovy = 45.0f) {
    camera::Params p;
    p.distance = distance;
    p.fovy = fovy;
    return p;
}

camera::Params ortho_params(float distance, float fovy = 22.0f) {
    camera::Params p;
    p.type = camera::Type::IsometricOrtho;
    p.distance = distance;
    p.fovy = fovy;
    return p;
}
} // namespace

TEST_CASE("create camera returns id and rejects duplicate names") {
    camera::Manager manager;

    const camera::Id id = manager.create("main", isometric_params(15.0f));
    REQUIRE(id != camera::kInvalidId);
    REQUIRE(manager.has(id));
    REQUIRE(manager.has("main"));

    REQUIRE(manager.create("main", isometric_params(10.0f)) == camera::kInvalidId);
}

TEST_CASE("cameras are addressable by both name and id") {
    camera::Manager manager;
    const camera::Id id = manager.create("main", isometric_params(15.0f));

    REQUIRE(manager.find("main") == id);
    REQUIRE(manager.find("missing") == camera::kInvalidId);
    REQUIRE(manager.names() == std::vector<std::string>{"main"});
}

TEST_CASE("active camera switches by id and name") {
    camera::Manager manager;
    const camera::Id a = manager.create("a", isometric_params(15.0f));
    const camera::Id b = manager.create("b", isometric_params(25.0f));

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
    manager.create("main", isometric_params(15.0f, 45.0f));
    manager.set_active("main");

    REQUIRE(manager.set_param_active("fov", 60.0));
    REQUIRE(manager.active_params()->fovy == 60.0f);

    // fovy 别名等价。
    REQUIRE(manager.set_param_active("fovy", 30.0));
    REQUIRE(manager.active_params()->fovy == 30.0f);
}

TEST_CASE("set_param distance repositions the isometric camera") {
    camera::Manager manager;
    manager.create("main", isometric_params(15.0f));
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
    manager.create("main", isometric_params(15.0f));
    manager.set_active("main");

    REQUIRE_FALSE(manager.set_param_active("nope", 1.0));
}

TEST_CASE("destroy removes camera and clears active when destroyed") {
    camera::Manager manager;
    const camera::Id a = manager.create("a", isometric_params(15.0f));
    manager.create("b", isometric_params(25.0f));
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

TEST_CASE("type name mapping is bidirectional") {
    REQUIRE(camera::type_name(camera::Type::Isometric) == std::string("isometric"));
    REQUIRE(camera::type_name(camera::Type::IsometricOrtho) == std::string("isometric_ortho"));

    REQUIRE(camera::type_from_name("isometric") == camera::Type::Isometric);
    REQUIRE(camera::type_from_name("isometric_ortho") == camera::Type::IsometricOrtho);
    REQUIRE_FALSE(camera::type_from_name("perspective").has_value());
    REQUIRE_FALSE(camera::type_from_name("").has_value());
}

TEST_CASE("create defaults projection by type and derives position") {
    camera::Manager manager;

    const camera::Id persp = manager.create("persp", isometric_params(15.0f));
    REQUIRE(persp != camera::kInvalidId);
    REQUIRE(manager.params(persp)->type == camera::Type::Isometric);
    REQUIRE(manager.params(persp)->projection == 0);

    const camera::Id ortho = manager.create("ortho", ortho_params(20.0f));
    REQUIRE(ortho != camera::kInvalidId);
    REQUIRE(manager.params(ortho)->type == camera::Type::IsometricOrtho);
    REQUIRE(manager.params(ortho)->projection == 1);
    REQUIRE(manager.params(ortho)->pos[0] == Catch::Approx(camera::kIsoDirX * 20.0f));
}

TEST_CASE("set_type switches type, resets projection and repositions") {
    camera::Manager manager;
    manager.create("main", isometric_params(15.0f));
    manager.set_active("main");
    manager.set_param_active("target_x", 4.0);
    manager.set_param_active("target_z", 6.0);
    manager.set_param_active("projection", 1); // 手动覆盖

    REQUIRE(manager.set_type_active(camera::Type::IsometricOrtho));
    const camera::Params* p = manager.active_params();
    REQUIRE(p->type == camera::Type::IsometricOrtho);
    REQUIRE(p->projection == 1);
    REQUIRE(p->pos[0] == Catch::Approx(4.0f + camera::kIsoDirX * 15.0f));

    REQUIRE(manager.set_type("main", camera::Type::Isometric));
    REQUIRE(p->type == camera::Type::Isometric);
    REQUIRE(p->projection == 0);

    REQUIRE_FALSE(manager.set_type("missing", camera::Type::Isometric));
}

TEST_CASE("set_follow stores state and validates smoothing") {
    camera::Manager manager;
    const camera::Id id = manager.create("main", isometric_params(15.0f));

    REQUIRE(manager.set_follow(id, 7, 0.2f));
    REQUIRE(manager.follow(id)->enabled);
    REQUIRE(manager.follow(id)->entity == 7);
    REQUIRE(manager.follow(id)->smoothing == 0.2f);
    REQUIRE(manager.follow(id)->pending_snap); // 首次绑定 → 下一 tick 直跳

    // smoothing <= 0 拒绝。
    REQUIRE_FALSE(manager.set_follow(id, 7, 0.0f));
    REQUIRE(manager.follow(id)->smoothing == 0.2f);

    // 清除跟随（entity 0 是合法实体，不能作清除语义）。
    REQUIRE(manager.clear_follow(id));
    REQUIRE_FALSE(manager.follow(id)->enabled);

    // 未知 id 返回 false。
    REQUIRE_FALSE(manager.set_follow(camera::kInvalidId, 1, 0.2f));
    REQUIRE_FALSE(manager.set_follow("missing", 1, 0.2f));
}

TEST_CASE("set_follow accepts entity 0 as a valid target") {
    // 回归: entt 首个创建实体 id 为 0, 跟随不能把 entity 0 当哨兵。
    camera::Manager manager;
    const camera::Id id = manager.create("main", isometric_params(15.0f));

    REQUIRE(manager.set_follow(id, 0, 0.2f));
    REQUIRE(manager.follow(id)->enabled);
    REQUIRE(manager.follow(id)->entity == 0);

    const float target[3] = {4.0f, 2.0f, 1.0f};
    REQUIRE(manager.follow_tick(id, target, 0.1f)); // snap 到目标
    REQUIRE(manager.params(id)->target[0] == Catch::Approx(4.0f));
}

TEST_CASE("set_follow same entity does not re-snap, new entity does") {
    camera::Manager manager;
    const camera::Id id = manager.create("main", isometric_params(15.0f));

    manager.set_follow(id, 7, 0.2f);
    const float any[3] = {0.0f, 0.0f, 0.0f};
    manager.follow_tick(id, any, 0.1f); // 消费 pending_snap
    REQUIRE_FALSE(manager.follow(id)->pending_snap);

    // 同 entity 重设 smoothing 不 snap。
    manager.set_follow(id, 7, 0.5f);
    REQUIRE_FALSE(manager.follow(id)->pending_snap);
    REQUIRE(manager.follow(id)->smoothing == 0.5f);

    // 换 entity 触发 snap。
    manager.set_follow(id, 9, 0.5f);
    REQUIRE(manager.follow(id)->pending_snap);
}

TEST_CASE("follow_tick snaps on first tick") {
    camera::Manager manager;
    const camera::Id id = manager.create("main", isometric_params(15.0f));
    manager.set_follow(id, 1, 0.2f);

    const float target[3] = {10.0f, 3.0f, -4.0f};
    REQUIRE(manager.follow_tick(id, target, 0.1f));

    const camera::Params* p = manager.params(id);
    REQUIRE(p->target[0] == Catch::Approx(10.0f));
    REQUIRE(p->target[1] == Catch::Approx(3.0f));
    REQUIRE(p->target[2] == Catch::Approx(-4.0f));
    // tick 后位置重推导。
    REQUIRE(p->pos[0] == Catch::Approx(10.0f + camera::kIsoDirX * 15.0f));
}

TEST_CASE("follow_tick smooths toward target and converges") {
    camera::Manager manager;
    const camera::Id id = manager.create("main", isometric_params(15.0f));
    manager.set_follow(id, 1, 0.2f);

    const float start[3] = {0.0f, 0.0f, 0.0f};
    manager.follow_tick(id, start, 0.1f); // snap 到 0

    const float target[3] = {5.0f, 2.0f, 1.0f};
    manager.follow_tick(id, target, 0.1f); // 第一步: 指数平滑, 未到位
    const camera::Params* p = manager.params(id);
    const float factor = camera::smooth_factor(0.2f, 0.1f);
    REQUIRE(p->target[0] == Catch::Approx(5.0f * factor));

    // 多步收敛于目标。
    for (int i = 0; i < 500; ++i) {
        manager.follow_tick(id, target, 0.1f);
    }
    REQUIRE(manager.params(id)->target[0] == Catch::Approx(5.0f).margin(0.001f));
    REQUIRE(manager.params(id)->target[2] == Catch::Approx(1.0f).margin(0.001f));
}

TEST_CASE("follow_tick is frame rate independent") {
    camera::Manager a;
    const camera::Id id_a = a.create("a", isometric_params(15.0f));
    a.set_follow(id_a, 1, 0.3f);

    camera::Manager b;
    const camera::Id id_b = b.create("b", isometric_params(15.0f));
    b.set_follow(id_b, 1, 0.3f);

    const float zero[3] = {0.0f, 0.0f, 0.0f};
    const float target[3] = {8.0f, 4.0f, 2.0f};
    a.follow_tick(id_a, zero, 0.1f); // snap
    b.follow_tick(id_b, zero, 0.1f); // snap

    // 相同平滑时长: dt=0.1 两步 vs dt=0.2 一步。
    a.follow_tick(id_a, target, 0.1f);
    a.follow_tick(id_a, target, 0.1f);
    b.follow_tick(id_b, target, 0.2f);

    for (int i = 0; i < 3; ++i) {
        REQUIRE(a.params(id_a)->target[i] == Catch::Approx(b.params(id_b)->target[i]));
    }
}

TEST_CASE("follow_tick respects clear_follow and invalid dt") {
    camera::Manager manager;
    const camera::Id id = manager.create("main", isometric_params(15.0f));
    manager.set_active(id);
    manager.set_follow(id, 1, 0.2f);

    const float target[3] = {1.0f, 2.0f, 3.0f};
    REQUIRE_FALSE(manager.follow_tick(id, target, 0.0f)); // dt <= 0 拒绝

    REQUIRE(manager.clear_follow(id));
    REQUIRE_FALSE(manager.follow(id)->enabled);
    REQUIRE_FALSE(manager.follow_tick(id, target, 0.1f)); // 无跟随

    // clear_follow 名称/活动重载。
    manager.set_follow("main", 3, 0.4f);
    REQUIRE(manager.follow(id)->entity == 3);
    REQUIRE(manager.clear_follow("main"));
    REQUIRE_FALSE(manager.follow(id)->enabled);

    manager.set_follow_active(5, 0.1f);
    REQUIRE(manager.follow(id)->entity == 5);
    REQUIRE(manager.clear_follow_active());
    REQUIRE_FALSE(manager.follow(id)->enabled);
}

TEST_CASE("follow_hard writes target directly without smoothing") {
    camera::Manager manager;
    const camera::Id id = manager.create("main", isometric_params(15.0f));
    manager.set_active(id);

    // 未启用跟随时返回 false。
    const float t0[3] = {1.0f, 1.0f, 1.0f};
    REQUIRE_FALSE(manager.follow_hard(id, t0));

    REQUIRE(manager.set_follow(id, 0, 0.2f)); // entity 0 合法

    const float target[3] = {12.0f, 3.0f, -5.0f};
    REQUIRE(manager.follow_hard(id, target));

    const camera::Params* p = manager.params(id);
    REQUIRE(p->target[0] == Catch::Approx(12.0f));
    REQUIRE(p->target[1] == Catch::Approx(3.0f));
    REQUIRE(p->target[2] == Catch::Approx(-5.0f));
    // 直写后位置重推导。
    REQUIRE(p->pos[0] == Catch::Approx(12.0f + camera::kIsoDirX * 15.0f));

    // 连续两次直写无插值 (第二步即完全到位)。
    const float target2[3] = {20.0f, 6.0f, -10.0f};
    REQUIRE(manager.follow_hard(id, target2));
    REQUIRE(p->target[0] == Catch::Approx(20.0f));

    // 清除跟随后返回 false。
    REQUIRE(manager.clear_follow(id));
    REQUIRE_FALSE(manager.follow_hard(id, target2));
}

TEST_CASE("snap_follow forces next tick jump") {
    camera::Manager manager;
    const camera::Id id = manager.create("main", isometric_params(15.0f));
    manager.set_active(id);
    manager.set_follow(id, 1, 0.2f);

    const float zero[3] = {0.0f, 0.0f, 0.0f};
    manager.follow_tick(id, zero, 0.1f); // 消费 snap
    REQUIRE_FALSE(manager.follow(id)->pending_snap);

    REQUIRE(manager.snap_follow(id));
    REQUIRE(manager.follow(id)->pending_snap);

    REQUIRE(manager.snap_follow_active());
    REQUIRE(manager.follow(id)->pending_snap);
}
