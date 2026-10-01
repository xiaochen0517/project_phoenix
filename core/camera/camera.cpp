#include "camera/camera.h"

#include "log/app_log.h"

#include <algorithm>
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

void apply_isometric(Params& params) {
    params.pos[0] = params.target[0] + kIsoDirX * params.distance;
    params.pos[1] = params.target[1] + kIsoDirY * params.distance;
    params.pos[2] = params.target[2] + kIsoDirZ * params.distance;
}

Manager::Instance* Manager::find_instance(Id id) {
    const auto it = std::find_if(instances_.begin(), instances_.end(),
                                 [id](const Instance& inst) { return inst.id == id; });
    return it == instances_.end() ? nullptr : &(*it);
}

const Manager::Instance* Manager::find_instance(Id id) const {
    const auto it = std::find_if(instances_.begin(), instances_.end(),
                                 [id](const Instance& inst) { return inst.id == id; });
    return it == instances_.end() ? nullptr : &(*it);
}

Manager::Instance* Manager::find_instance(const std::string& name) {
    const auto it = std::find_if(instances_.begin(), instances_.end(),
                                 [&name](const Instance& inst) { return inst.name == name; });
    return it == instances_.end() ? nullptr : &(*it);
}

const Manager::Instance* Manager::find_instance(const std::string& name) const {
    const auto it = std::find_if(instances_.begin(), instances_.end(),
                                 [&name](const Instance& inst) { return inst.name == name; });
    return it == instances_.end() ? nullptr : &(*it);
}

Id Manager::create(const std::string& name, Type type, const Params& params) {
    if (find_instance(name) != nullptr) {
        app_log::error("camera: create failed, duplicate name: " + name);
        return kInvalidId;
    }
    Instance inst;
    inst.id = next_id_++;
    inst.name = name;
    inst.type = type;
    inst.params = params;
    if (type == Type::Isometric) {
        apply_isometric(inst.params);
    }
    instances_.push_back(inst);
    return inst.id;
}

bool Manager::destroy(Id id) {
    const auto it = std::find_if(instances_.begin(), instances_.end(),
                                 [id](const Instance& inst) { return inst.id == id; });
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
    if (inst->type == Type::Isometric) {
        apply_isometric(inst->params);
    }
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
} // namespace camera
