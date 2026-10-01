#include "util/asset_path.h"

#include "raylib.h"

namespace asset_path {
std::string resolve(const std::string& requested) {
    if (FileExists(requested.c_str())) {
        return requested;
    }

    std::string prefix;
    for (int i = 0; i < 4; ++i) {
        prefix += "../";
        const std::string candidate = prefix + requested;
        if (FileExists(candidate.c_str())) {
            return candidate;
        }
    }

    const std::string candidate = GetApplicationDirectory() + requested;
    if (FileExists(candidate.c_str())) {
        return candidate;
    }

    return requested; // 未找到, 交给加载器统一报错
}
} // namespace asset_path
