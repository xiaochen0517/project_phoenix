#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace camera {
using Id = std::uint32_t;
inline constexpr Id kInvalidId = 0;

// 相机类型（可扩展；P0 仅 isometric 验证载体）。
enum class Type {
    Isometric,
};

// isometric 等距视角方向 = normalize(1,1,1) ≈ (0.57735, 0.57735, 0.57735)。
inline constexpr float kIsoDirX = 0.5773502692f;
inline constexpr float kIsoDirY = 0.5773502692f;
inline constexpr float kIsoDirZ = 0.5773502692f;

// 相机参数（纯数据，语义对齐 raylib Camera3D；不依赖 raylib）。
struct Params {
    float pos[3]{8.0f, 6.0f, 10.0f};
    float target[3]{0.0f, 0.0f, 0.0f};
    float up[3]{0.0f, 1.0f, 0.0f};
    float fovy = 45.0f;
    int projection = 0; // 0 = 透视(CAMERA_PERSPECTIVE), 1 = 正交(CAMERA_ORTHOGRAPHIC)
    float distance = 10.0f; // 相机到目标点距离（isometric 用于推导位置）
};

// isometric 位置推导: position = target + kIsoDir * distance（纯函数，可单测）。
void apply_isometric(Params& params);

// 多相机实例管理：名称唯一，同时支持名称与 id 两种标识。
class Manager {
  public:
    // 创建相机，返回 id；名称冲突返回 kInvalidId。
    Id create(const std::string& name, Type type, const Params& params);
    bool destroy(Id id);
    bool destroy(const std::string& name);
    bool has(Id id) const;
    bool has(const std::string& name) const;

    Id find(const std::string& name) const; // 名称→id，不存在返回 kInvalidId
    std::vector<std::string> names() const; // list

    bool set_active(Id id);
    bool set_active(const std::string& name);
    Id active() const;               // 无活动相机返回 kInvalidId
    std::string active_name() const; // 无活动相机返回空串

    const Params* params(Id id) const;
    const Params* active_params() const;

    // 按 key 修改参数（作用于指定相机 / 活动相机）；未知 key 返回 false。
    bool set_param(Id id, const std::string& key, double value);
    bool set_param(const std::string& name, const std::string& key, double value);
    bool set_param_active(const std::string& key, double value);

  private:
    struct Instance {
        Id id;
        std::string name;
        Type type;
        Params params;
    };

    Instance* find_instance(Id id);
    const Instance* find_instance(Id id) const;
    Instance* find_instance(const std::string& name);
    const Instance* find_instance(const std::string& name) const;

    std::vector<Instance> instances_;
    Id next_id_ = 1;
    Id active_ = kInvalidId;
};
} // namespace camera
