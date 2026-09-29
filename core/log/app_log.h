#pragma once

#include <string>

namespace app_log {
    void init();

    void debug(const std::string &message);

    void info(const std::string &message);

    void warn(const std::string &message);

    void error(const std::string &message);

    void critical(const std::string &message);
}
