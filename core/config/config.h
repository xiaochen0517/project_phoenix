#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace config {
// 配置文档: 包装一份已解析的 JSON 对象, 提供类型安全读取与校验。
// 采用 pimpl 隔离 nlohmann/json 头文件 (ADR-5), 公开头不暴露第三方类型。
class Document {
  public:
    Document(); // 空文档 (无字段)
    ~Document();

    Document(Document&&) noexcept;
    Document& operator=(Document&&) noexcept;
    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;

    // 按点分隔路径读取 (如 "window.width"), 字段不存在或类型不匹配时返回默认值。
    bool get_bool(const std::string& path, bool def) const;
    std::int64_t get_int(const std::string& path, std::int64_t def) const;
    double get_double(const std::string& path, double def) const;
    std::string get_string(const std::string& path, std::string def) const;

    // 校验: 必填字段 + 可选字段; 返回缺失的必填字段与未声明的未知字段。
    struct ValidationResult {
        std::vector<std::string> missing; // 必填但缺失
        std::vector<std::string> unknown; // 存在但未在 required/optional 声明
        bool ok() const {
            return missing.empty() && unknown.empty();
        }
    };
    ValidationResult validate(const std::vector<std::string>& required,
                              const std::vector<std::string>& optional = {}) const;

    // 列出指定点分隔路径下对象的子键名; 节点不存在或非对象返回空列表。
    std::vector<std::string> keys(const std::string& path) const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    friend std::optional<Document> load_file(const std::string& path);
};

// 从文件加载 JSON 文档; 文件不存在或解析失败时返回 std::nullopt (经 app_log::error 输出)。
std::optional<Document> load_file(const std::string& path);
} // namespace config
