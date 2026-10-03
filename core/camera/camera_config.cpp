#include "camera/camera_config.h"

#include "config/config.h"
#include "log/app_log.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace camera {
std::optional<Config> load_config(const config::Document& doc) {
    const std::int64_t count = doc.size("cameras");
    if (count < 0) {
        app_log::error("camera_config: missing 'cameras' array");
        return std::nullopt;
    }

    Config config;
    config.cameras.reserve(static_cast<std::size_t>(count));

    for (std::int64_t i = 0; i < count; ++i) {
        const std::string p = "cameras." + std::to_string(i);

        CameraEntry entry;
        entry.name = doc.get_string(p + ".name", "");
        if (entry.name.empty()) {
            app_log::error("camera_config: cameras[" + std::to_string(i) + "].name missing or empty");
            return std::nullopt;
        }

        const std::string type_str = doc.get_string(p + ".type", "isometric");
        const std::optional<Type> type = type_from_name(type_str);
        if (!type.has_value()) {
            app_log::error("camera_config: cameras[" + std::to_string(i) + "].type unknown: " + type_str);
            return std::nullopt;
        }

        entry.params.type = *type;
        entry.params.distance = static_cast<float>(doc.get_double(p + ".distance", 10.0));
        entry.params.fovy = static_cast<float>(doc.get_double(p + ".fovy", 45.0));

        // follow 为对象时启用跟随（size 0 表示字段缺失或空对象，均视为未启用）。
        if (doc.size(p + ".follow") > 0) {
            entry.follow_enabled = true;
            entry.smoothing = static_cast<float>(doc.get_double(p + ".follow.smoothing", 0.0));
            if (entry.smoothing <= 0.0f) {
                app_log::error("camera_config: cameras[" + std::to_string(i) + "].follow.smoothing must be > 0");
                return std::nullopt;
            }
        }

        config.cameras.push_back(std::move(entry));
    }

    if (config.cameras.empty()) {
        app_log::error("camera_config: 'cameras' array is empty");
        return std::nullopt;
    }

    config.active = doc.get_string("active", config.cameras.front().name);
    bool active_found = false;
    for (const CameraEntry& entry : config.cameras) {
        if (entry.name == config.active) {
            active_found = true;
            break;
        }
    }
    if (!active_found) {
        app_log::warn("camera_config: active '" + config.active + "' not found, fallback to first camera");
        config.active = config.cameras.front().name;
    }
    return config;
}
} // namespace camera
