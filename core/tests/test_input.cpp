#include "config/config.h"
#include "input/input.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

namespace {
std::string config_path(const char* name) {
    return std::string(PROJECT_PHOENIX_CONFIG_DIR) + "/" + name;
}
} // namespace

TEST_CASE("key name conversion is bidirectional") {
    REQUIRE(input::key_from_name("W") == input::Key::W);
    REQUIRE(input::key_from_name("w") == input::Key::W); // 小写也接受
    REQUIRE(input::key_from_name("SPACE") == input::Key::Space);
    REQUIRE(input::key_from_name("LEFT_SHIFT") == input::Key::LeftShift);
    REQUIRE(input::key_from_name("0") == input::Key::D0);
    REQUIRE(input::key_from_name("9") == input::Key::D9);
    REQUIRE(input::key_from_name("NOT_A_KEY") == input::Key::Unknown);

    REQUIRE(input::key_name(input::Key::W) == "W");
    REQUIRE(input::key_name(input::Key::Space) == "SPACE");
    REQUIRE(input::key_name(input::Key::LeftShift) == "LEFT_SHIFT");
    REQUIRE(input::key_name(input::Key::D0) == "0");
    REQUIRE(input::key_name(input::Key::Unknown) == "UNKNOWN");
}

TEST_CASE("bind, unbind, and query actions") {
    input::BindingMap map;
    REQUIRE(map.bind("move_up", input::Key::W) == input::BindResult::Ok);
    REQUIRE(map.query("move_up").has_value());
    REQUIRE(*map.query("move_up") == input::Key::W);
    REQUIRE_FALSE(map.query("move_down").has_value());

    REQUIRE(map.unbind("move_up"));
    REQUIRE_FALSE(map.query("move_up").has_value());
    REQUIRE_FALSE(map.unbind("move_up")); // 已解绑
}

TEST_CASE("static conflict detection rejects duplicate key") {
    input::BindingMap map;
    REQUIRE(map.bind("move_up", input::Key::W) == input::BindResult::Ok);
    REQUIRE(map.bind("move_down", input::Key::W) == input::BindResult::Conflict);

    // 冲突时原绑定保留。
    REQUIRE(map.query("move_up").has_value());
    REQUIRE_FALSE(map.query("move_down").has_value());

    // force 顶替原动作。
    REQUIRE(map.bind("move_down", input::Key::W, true) == input::BindResult::Ok);
    REQUIRE_FALSE(map.query("move_up").has_value());
    REQUIRE(map.query("move_down").has_value());
}

TEST_CASE("reverse query returns actions bound to a key") {
    input::BindingMap map;
    map.bind("move_up", input::Key::W);
    map.bind("jump", input::Key::Space);

    REQUIRE(map.actions_for(input::Key::W) == std::vector<std::string>{"move_up"});
    REQUIRE(map.actions_for(input::Key::Space) == std::vector<std::string>{"jump"});
    REQUIRE(map.actions_for(input::Key::A).empty());
}

TEST_CASE("sample state machine produces edge and hold states") {
    input::Manager manager;
    manager.context("main_game").bind("move_up", input::Key::W);
    manager.set_context("main_game");

    REQUIRE(manager.state("move_up") == input::ActionState::Idle);

    manager.sample({input::Key::W});
    REQUIRE(manager.state("move_up") == input::ActionState::Pressed);

    manager.sample({input::Key::W});
    REQUIRE(manager.state("move_up") == input::ActionState::Held);

    manager.sample({});
    REQUIRE(manager.state("move_up") == input::ActionState::Released);

    manager.sample({});
    REQUIRE(manager.state("move_up") == input::ActionState::Idle);
}

TEST_CASE("contexts isolate bindings by scene") {
    input::Manager manager;
    manager.context("main_game").bind("move_up", input::Key::W);
    manager.context("menu").bind("move_up", input::Key::Up);

    REQUIRE(manager.set_context("main_game"));
    REQUIRE(*manager.query("move_up") == input::Key::W);

    REQUIRE(manager.set_context("menu"));
    REQUIRE(*manager.query("move_up") == input::Key::Up);

    REQUIRE_FALSE(manager.set_context("missing"));
}

TEST_CASE("action_to_dir returns normalized direction vectors") {
    const float k = 0.70710678f;

    const auto up = input::action_to_dir("move_up");
    REQUIRE(up.has_value());
    REQUIRE(up->x == Catch::Approx(-k));
    REQUIRE(up->y == Catch::Approx(-k));

    const auto right = input::action_to_dir("move_right");
    REQUIRE(right.has_value());
    REQUIRE(right->x == Catch::Approx(k));
    REQUIRE(right->y == Catch::Approx(-k));

    REQUIRE_FALSE(input::action_to_dir("jump").has_value());
}

TEST_CASE("load_bindings reads contexts from config document") {
    const auto doc = config::load_file(config_path("test_input_bindings.json"));
    REQUIRE(doc.has_value());

    input::Manager manager;
    input::load_bindings(manager, *doc);

    // 默认上下文为 main_game。
    REQUIRE(manager.active_context() == "main_game");
    REQUIRE(*manager.query("move_up") == input::Key::W);
    REQUIRE(*manager.query("move_down") == input::Key::S);

    // 第二上下文 menu 独立绑定 (含方向键)。
    REQUIRE(manager.set_context("menu"));
    REQUIRE(*manager.query("move_up") == input::Key::Up);
    REQUIRE(*manager.query("confirm") == input::Key::Enter);

    // 非法键名被跳过, 不产生绑定。
    REQUIRE_FALSE(manager.query("bad_key").has_value());
}
