#include "script/sol2_bridge.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("sol2_bridge::self_test reports all checks passed") {
    const std::string report = sol2_bridge::self_test();
    REQUIRE(report.find("6 passed, 0 failed") != std::string::npos);
}
