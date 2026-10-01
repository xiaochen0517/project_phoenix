#include "config/config.h"

#include "log/app_log.h"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace config {
namespace {
// 按 '.' 拆分点分隔路径, 返回每一段 (空段作为普通 key 处理, 一般匹配不到, 视为缺失)。
std::vector<std::string> split_path(const std::string& path) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= path.size()) {
        const std::size_t dot = path.find('.', start);
        const std::size_t end = (dot == std::string::npos) ? path.size() : dot;
        parts.push_back(path.substr(start, end - start));
        if (dot == std::string::npos) {
            break;
        }
        start = dot + 1;
    }
    return parts;
}

// 沿点分隔路径查找节点; 中途遇到非对象或 key 缺失返回 nullptr。
const nlohmann::json* find_node(const nlohmann::json& root, const std::string& path) {
    const std::vector<std::string> parts = split_path(path);
    const nlohmann::json* current = &root;
    for (const std::string& part : parts) {
        if (!current->is_object()) {
            return nullptr;
        }
        const auto it = current->find(part);
        if (it == current->end()) {
            return nullptr;
        }
        current = &(*it);
    }
    return current;
}
} // namespace

// ---- Document ----
struct Document::Impl {
    nlohmann::json root;
};

Document::Document() : impl_(std::make_unique<Impl>()) {
}

Document::~Document() = default;

Document::Document(Document&&) noexcept = default;

Document& Document::operator=(Document&&) noexcept = default;

bool Document::get_bool(const std::string& path, bool def) const {
    const nlohmann::json* node = find_node(impl_->root, path);
    if (node == nullptr || !node->is_boolean()) {
        return def;
    }
    return node->get<bool>();
}

std::int64_t Document::get_int(const std::string& path, std::int64_t def) const {
    const nlohmann::json* node = find_node(impl_->root, path);
    if (node == nullptr || !node->is_number_integer()) {
        return def;
    }
    return node->get<std::int64_t>();
}

double Document::get_double(const std::string& path, double def) const {
    const nlohmann::json* node = find_node(impl_->root, path);
    if (node == nullptr || !node->is_number()) {
        return def;
    }
    return node->get<double>();
}

std::string Document::get_string(const std::string& path, std::string def) const {
    const nlohmann::json* node = find_node(impl_->root, path);
    if (node == nullptr || !node->is_string()) {
        return def;
    }
    return node->get<std::string>();
}

Document::ValidationResult Document::validate(const std::vector<std::string>& required,
                                              const std::vector<std::string>& optional) const {
    ValidationResult result;

    for (const std::string& key : required) {
        if (find_node(impl_->root, key) == nullptr) {
            result.missing.push_back(key);
        }
    }

    const auto declared = [&required, &optional](const std::string& key) {
        return std::find(required.begin(), required.end(), key) != required.end() ||
               std::find(optional.begin(), optional.end(), key) != optional.end();
    };

    if (impl_->root.is_object()) {
        for (auto it = impl_->root.begin(); it != impl_->root.end(); ++it) {
            if (!declared(it.key())) {
                result.unknown.push_back(it.key());
            }
        }
    }

    return result;
}

// ---- load_file ----
std::optional<Document> load_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        app_log::error("config: cannot open file: " + path);
        return std::nullopt;
    }

    // 非异常解析 (项目未启用 /EHsc, 避免依赖 try/catch 捕获 parse_error);
    // nlohmann 3.x 的 parse 在 allow_exceptions=false 时返回 discarded 对象表示失败。
    nlohmann::json parsed = nlohmann::json::parse(file, nullptr, false);
    if (parsed.is_discarded()) {
        app_log::error("config: parse error in file: " + path);
        return std::nullopt;
    }

    Document doc;
    doc.impl_->root = std::move(parsed);
    return doc;
}
} // namespace config
