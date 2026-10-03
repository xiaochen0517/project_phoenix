#include "camera/camera.h"

#include "log/app_log.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace camera {
namespace {
// 按 key 写入单个标量参数；未知 key 返回 false（不做 isometric 联动，联动由调用方处理）。
bool apply_key(Params& params, const std::string& key, double value) {
    if (key == "fov" || key == "fovy") {
        params.fovy = static_cast<float>(value);
        return true;
    }
    if (key == "distance") {
        params.distance = static_cast<float>(value);
        return true;
    }
    if (key == "pos_x") {
        params.pos[0] = static_cast<float>(value);
        return true;
    }
    if (key == "pos_y") {
        params.pos[1] = static_cast<float>(value);
        return true;
    }
    if (key == "pos_z") {
        params.pos[2] = static_cast<float>(value);
        return true;
    }
    if (key == "target_x") {
        params.target[0] = static_cast<float>(value);
        return true;
    }
    if (key == "target_y") {
        params.target[1] = static_cast<float>(value);
        return true;
    }
    if (key == "target_z") {
        params.target[2] = static_cast<float>(value);
        return true;
    }
    if (key == "up_x") {
        params.up[0] = static_cast<float>(value);
        return true;
    }
    if (key == "up_y") {
        params.up[1] = static_cast<float>(value);
        return true;
    }
    if (key == "up_z") {
        params.up[2] = static_cast<float>(value);
        return true;
    }
    if (key == "projection") {
        params.projection = static_cast<int>(value);
        return true;
    }
    return false;
}
} // namespace

std::optional<Type> type_from_name(const std::string& name) {
    if (name == "isometric") {
        return Type::Isometric;
    }
    if (name == "isometric_ortho") {
        return Type::IsometricOrtho;
    }
    return std::nullopt;
}

const char* type_name(Type type) {
    switch (type) {
    case Type::Isometric:
        return "isometric";
    case Type::IsometricOrtho:
        return "isometric_ortho";
    }
    return "unknown"; // 不可达，防御兜底
}

void apply_isometric(Params& params) {
    params.pos[0] = params.target[0] + kIsoDirX * params.distance;
    params.pos[1] = params.target[1] + kIsoDirY * params.distance;
    params.pos[2] = params.target[2] + kIsoDirZ * params.distance;
}

float smooth_factor(float smoothing, float dt) {
    return 1.0f - std::exp(-smoothing * dt);
}

void smooth_step(float cur[3], const float target[3], float factor) {
    for (int i = 0; i < 3; ++i) {
        cur[i] += (target[i] - cur[i]) * factor;
    }
}

Manager::Instance* Manager::find_instance(Id id) {
    const auto it =
        std::find_if(instances_.begin(), instances_.end(), [id](const Instance& inst) { return inst.id == id; });
    return it == instances_.end() ? nullptr : &(*it);
}

const Manager::Instance* Manager::find_instance(Id id) const {
    const auto it =
        std::find_if(instances_.begin(), instances_.end(), [id](const Instance& inst) { return inst.id == id; });
    return it == instances_.end() ? nullptr : &(*it);
}

Manager::Instance* Manager::find_instance(const std::string& name) {
    const auto it =
        std::find_if(instances_.begin(), instances_.end(), [&name](const Instance& inst) { return inst.name == name; });
    return it == instances_.end() ? nullptr : &(*it);
}

const Manager::Instance* Manager::find_instance(const std::string& name) const {
    const auto it =
        std::find_if(instances_.begin(), instances_.end(), [&name](const Instance& inst) { return inst.name == name; });
    return it == instances_.end() ? nullptr : &(*it);
}

Id Manager::create(const std::string& name, const Params& params) {
    if (find_instance(name) != nullptr) {
        app_log::error("camera: create failed, duplicate name: " + name);
        return kInvalidId;
    }
    Instance inst;
    inst.id = next_id_++;
    inst.name = name;
    inst.params = params;
    // 按类型写默认投影（透视为 0，正交为 1），调用方仍可经 set_param("projection") 覆盖。
    inst.params.projection = (inst.params.type == Type::IsometricOrtho) ? 1 : 0;
    apply_isometric(inst.params);
    instances_.push_back(inst);
    return inst.id;
}

bool Manager::destroy(Id id) {
    const auto it =
        std::find_if(instances_.begin(), instances_.end(), [id](const Instance& inst) { return inst.id == id; });
    if (it == instances_.end()) {
        return false;
    }
    if (active_ == id) {
        active_ = kInvalidId;
    }
    instances_.erase(it);
    return true;
}

bool Manager::destroy(const std::string& name) {
    const Instance* inst = find_instance(name);
    if (inst == nullptr) {
        return false;
    }
    return destroy(inst->id);
}

bool Manager::has(Id id) const {
    return find_instance(id) != nullptr;
}

bool Manager::has(const std::string& name) const {
    return find_instance(name) != nullptr;
}

Id Manager::find(const std::string& name) const {
    const Instance* inst = find_instance(name);
    return inst == nullptr ? kInvalidId : inst->id;
}

std::vector<std::string> Manager::names() const {
    std::vector<std::string> out;
    out.reserve(instances_.size());
    for (const Instance& inst : instances_) {
        out.push_back(inst.name);
    }
    return out;
}

bool Manager::set_active(Id id) {
    if (find_instance(id) == nullptr) {
        return false;
    }
    active_ = id;
    return true;
}

bool Manager::set_active(const std::string& name) {
    const Instance* inst = find_instance(name);
    if (inst == nullptr) {
        return false;
    }
    active_ = inst->id;
    return true;
}

Id Manager::active() const {
    return active_;
}

std::string Manager::active_name() const {
    const Instance* inst = find_instance(active_);
    return inst == nullptr ? std::string() : inst->name;
}

const Params* Manager::params(Id id) const {
    const Instance* inst = find_instance(id);
    return inst == nullptr ? nullptr : &inst->params;
}

const Params* Manager::active_params() const {
    return params(active_);
}

bool Manager::set_param(Id id, const std::string& key, double value) {
    Instance* inst = find_instance(id);
    if (inst == nullptr) {
        app_log::error("camera: set_param failed, unknown id");
        return false;
    }
    if (!apply_key(inst->params, key, value)) {
        app_log::error("camera: set_param unknown key: " + key);
        return false;
    }
    // 两类 isometric 相机均以 target + distance 推导位置，改参后重推导。
    apply_isometric(inst->params);
    return true;
}

bool Manager::set_param(const std::string& name, const std::string& key, double value) {
    Instance* inst = find_instance(name);
    if (inst == nullptr) {
        app_log::error("camera: set_param failed, unknown name: " + name);
        return false;
    }
    return set_param(inst->id, key, value);
}

bool Manager::set_param_active(const std::string& key, double value) {
    return set_param(active_, key, value);
}

bool Manager::set_type(Id id, Type type) {
    Instance* inst = find_instance(id);
    if (inst == nullptr) {
        app_log::error("camera: set_type failed, unknown id");
        return false;
    }
    inst->params.type = type;
    inst->params.projection = (type == Type::IsometricOrtho) ? 1 : 0;
    apply_isometric(inst->params);
    return true;
}

bool Manager::set_type(const std::string& name, Type type) {
    Instance* inst = find_instance(name);
    if (inst == nullptr) {
        app_log::error("camera: set_type failed, unknown name: " + name);
        return false;
    }
    return set_type(inst->id, type);
}

bool Manager::set_type_active(Type type) {
    return set_type(active_, type);
}

bool Manager::set_follow(Id id, std::uint32_t entity, float smoothing) {
    Instance* inst = find_instance(id);
    if (inst == nullptr) {
        app_log::error("camera: set_follow failed, unknown id");
        return false;
    }
    if (smoothing <= 0.0f) {
        app_log::error("camera: set_follow failed, smoothing must be > 0");
        return false;
    }
    // 首次绑定或换实体 → 下一 tick 直跳（entity 0 是合法实体，不能作哨兵）。
    if (!inst->follow.enabled || inst->follow.entity != entity) {
        inst->follow.pending_snap = true;
    }
    inst->follow.enabled = true;
    inst->follow.entity = entity;
    inst->follow.smoothing = smoothing;
    return true;
}

bool Manager::set_follow(const std::string& name, std::uint32_t entity, float smoothing) {
    Instance* inst = find_instance(name);
    if (inst == nullptr) {
        app_log::error("camera: set_follow failed, unknown name: " + name);
        return false;
    }
    return set_follow(inst->id, entity, smoothing);
}

bool Manager::set_follow_active(std::uint32_t entity, float smoothing) {
    return set_follow(active_, entity, smoothing);
}

bool Manager::clear_follow(Id id) {
    Instance* inst = find_instance(id);
    if (inst == nullptr) {
        return false;
    }
    inst->follow.enabled = false;
    inst->follow.entity = 0;
    inst->follow.smoothing = 0.0f;
    inst->follow.pending_snap = false;
    return true;
}

bool Manager::clear_follow(const std::string& name) {
    Instance* inst = find_instance(name);
    if (inst == nullptr) {
        return false;
    }
    return clear_follow(inst->id);
}

bool Manager::clear_follow_active() {
    return clear_follow(active_);
}

bool Manager::snap_follow(Id id) {
    Instance* inst = find_instance(id);
    if (inst == nullptr || !inst->follow.enabled) {
        return false;
    }
    inst->follow.pending_snap = true;
    return true;
}

bool Manager::snap_follow_active() {
    return snap_follow(active_);
}

const Follow* Manager::follow(Id id) const {
    const Instance* inst = find_instance(id);
    return inst == nullptr ? nullptr : &inst->follow;
}

bool Manager::follow_tick(Id id, const float target[3], float dt) {
    Instance* inst = find_instance(id);
    if (inst == nullptr || !inst->follow.enabled || dt <= 0.0f) {
        return false;
    }
    if (inst->follow.pending_snap) {
        inst->follow.pending_snap = false;
        inst->params.target[0] = target[0];
        inst->params.target[1] = target[1];
        inst->params.target[2] = target[2];
    } else {
        smooth_step(inst->params.target, target, smooth_factor(inst->follow.smoothing, dt));
    }
    apply_isometric(inst->params);
    return true;
}

bool Manager::follow_hard(Id id, const float target[3]) {
    Instance* inst = find_instance(id);
    if (inst == nullptr || !inst->follow.enabled) {
        return false;
    }
    inst->follow.pending_snap = false; // 硬跟随无平滑, snap 概念不适用, 保持状态干净
    inst->params.target[0] = target[0];
    inst->params.target[1] = target[1];
    inst->params.target[2] = target[2];
    apply_isometric(inst->params);
    return true;
}
} // namespace camera
