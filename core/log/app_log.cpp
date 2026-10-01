#include "log/app_log.h"

#include <spdlog/spdlog.h>

namespace app_log {
void init() {
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::debug);
}

void debug(const std::string& message) {
    spdlog::debug("{}", message);
}

void info(const std::string& message) {
    spdlog::info("{}", message);
}

void warn(const std::string& message) {
    spdlog::warn("{}", message);
}

void error(const std::string& message) {
    spdlog::error("{}", message);
}

void critical(const std::string& message) {
    spdlog::critical("{}", message);
}
} // namespace app_log
