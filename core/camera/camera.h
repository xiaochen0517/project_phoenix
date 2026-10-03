#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace camera {
using Id = std::uint32_t;
inline constexpr Id kInvalidId = 0;

// 相机类型（isometric 的两种投影变体）。
enum class Type {
    Isometric,      // 透视 isometric（projection 默认 0，fovy = 视场角度数）
    IsometricOrtho, // 正交 isometric（projection 默认 1，fovy = 垂直视野世界单位）
};

// 类型名映射（JSON 配置 / Lua 绑定 / UI 共用）。
// "isometric" / "isometric_ortho"；未知名称返回 std::nullopt。
std::optional<Type> type_from_name(const std::string& name);
const char* type_name(Type type);

// isometric 等距视角方向 = normalize(1,1,1) ≈ (0.57735, 0.57735, 0.57735)。
inline constexpr float kIsoDirX = 0.5773502692f;
inline constexpr float kIsoDirY = 0.5773502692f;
inline constexpr float kIsoDirZ = 0.5773502692f;

// 相机参数（纯数据，语义对齐 raylib Camera3D；不依赖 raylib）。
struct Params {
    Type type = Type::Isometric; // 相机类型（单一事实来源，create/set_type 读写）
    float pos[3]{8.0f, 6.0f, 10.0f};
    float target[3]{0.0f, 0.0f, 0.0f};
    float up[3]{0.0f, 1.0f, 0.0f};
    float fovy = 45.0f;
    int projection = 0;     // 0 = 透视(CAMERA_PERSPECTIVE), 1 = 正交(CAMERA_ORTHOGRAPHIC)
    float distance = 10.0f; // 相机到目标点距离（isometric 用于推导位置）
};

// isometric 位置推导: position = target + kIsoDir * distance（纯函数，可单测）。
void apply_isometric(Params& params);

// 平滑纯函数（可单测）：帧率无关指数平滑。
// smooth_factor = 1 - exp(-smoothing * dt)；smooth_step: cur = lerp(cur, target, factor)。
float smooth_factor(float smoothing, float dt);
void smooth_step(float cur[3], const float target[3], float factor);

// 跟随状态（纯数据）。
struct Follow {
    bool enabled = false;      // 是否启用跟随（entt entity 0 是合法实体，不能以 entity==0 作哨兵）
    std::uint32_t entity = 0;  // 目标实体（entt entity 的 uint32 语义，本模块不 include entt）
    float smoothing = 0.0f;    // 指数平滑速率（> 0，越大跟随越快）
    bool pending_snap = false; // 置位后下一 tick 直接跳到位（首次绑定/切换 entity/强制回位）
};

// 多相机实例管理：名称唯一，同时支持名称与 id 两种标识。
class Manager {
  public:
    // 创建相机，返回 id；名称冲突返回 kInvalidId。
    // type 取自 params.type；按类型写默认 projection（Isometric→0 / IsometricOrtho→1）并推导位置。
    Id create(const std::string& name, const Params& params);
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

    // 切换类型：写 Params.type → 重置 projection 为类型默认 → 重推导位置。
    bool set_type(Id id, Type type);
    bool set_type(const std::string& name, Type type);
    bool set_type_active(Type type);

    // 设置跟随：smoothing <= 0 返回 false；首次绑定或 entity 变化置 pending_snap（下一 tick 直跳），
    // 同 entity 重设 smoothing 不 snap。entity 为 uint32 语义（0 是合法实体，清除跟随用 clear_follow）。
    bool set_follow(Id id, std::uint32_t entity, float smoothing);
    bool set_follow(const std::string& name, std::uint32_t entity, float smoothing);
    bool set_follow_active(std::uint32_t entity, float smoothing);

    // 清除跟随（follow 状态归零，目标保持当前位置）。
    bool clear_follow(Id id);
    bool clear_follow(const std::string& name);
    bool clear_follow_active();

    // 强制下一 tick 直跳到位（如退出自由相机回位）；返回 false 表示相机不存在。
    bool snap_follow(Id id);
    bool snap_follow_active();

    // 跟随状态查询；相机不存在返回 nullptr（存在但未启用时 enabled == false）。
    const Follow* follow(Id id) const;

    // 每 tick 平滑跟随：pending_snap 直写目标否则 smooth_step，随后重推导位置；
    // 无跟随 / dt <= 0 返回 false。（指数平滑能力；world_scene 当前用 follow_hard 实时跟随）
    bool follow_tick(Id id, const float target[3], float dt);

    // 实时跟随（无平滑）: target 直写 + 重推导位置; 未启用跟随 / 相机不存在返回 false。
    bool follow_hard(Id id, const float target[3]);

  private:
    struct Instance {
        Id id;
        std::string name;
        Params params;
        Follow follow;
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
