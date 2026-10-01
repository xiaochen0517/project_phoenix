#include "log/app_log.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("app_log smoke test logs at all levels without crashing") {
    app_log::init();
    app_log::debug("unit test debug");
    app_log::info("unit test info");
    app_log::warn("unit test warn");
    app_log::error("unit test error");
    app_log::critical("unit test critical");
    SUCCEED();
}
