#include <catch2/catch_test_case_info.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <iostream>

namespace {
    // 每个测试用例结束后输出统一成功提示 (含测试名), 便于人工快速判断每个用例的通过情况。
    struct SuccessListener : Catch::EventListenerBase {
        using Catch::EventListenerBase::EventListenerBase;

        void testCaseEnded(const Catch::TestCaseStats &testCaseStats) override {
            if (testCaseStats.aborting || testCaseStats.totals.assertions.failed != 0) {
                return;
            }
            std::cout << "[PASS] " << testCaseStats.testInfo->name << std::endl;
        }
    };
} // namespace

CATCH_REGISTER_LISTENER(SuccessListener)
