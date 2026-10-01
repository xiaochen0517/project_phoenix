#pragma once

#include <functional>
#include <utility>
#include <vector>

#include <entt/entt.hpp>

namespace ecs {
// Entity: 直接采用 entt 原生实体句柄 (uint32), 为后续 Lua 侧 number 互转保留语义。
using Entity = entt::entity;
inline constexpr Entity null = entt::null;

// Registry: entt::registry 的薄封装, 屏蔽第三方 API 细节, 应用层只接触 ecs:: 类型。
class Registry {
  public:
    // entity 创建 / 销毁 / 存活判定。
    Entity create() {
        return registry_.create();
    }

    void destroy(Entity entity) {
        registry_.destroy(entity);
    }

    bool alive(Entity entity) const {
        return registry_.valid(entity);
    }

    // component: attach(默认 emplace) / attach_or_replace / detach / has / get。
    template <typename T, typename... Args>
    T& attach(Entity entity, Args&&... args) {
        return registry_.emplace<T>(entity, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    T& attach_or_replace(Entity entity, Args&&... args) {
        return registry_.emplace_or_replace<T>(entity, std::forward<Args>(args)...);
    }

    template <typename T>
    void detach(Entity entity) {
        registry_.remove<T>(entity);
    }

    template <typename T>
    bool has(Entity entity) const {
        return registry_.all_of<T>(entity);
    }

    template <typename T>
    T& get(Entity entity) {
        return registry_.get<T>(entity);
    }

    template <typename T>
    const T& get(Entity entity) const {
        return registry_.get<T>(entity);
    }

    // view: 按组件组合查询, 透传 entt view (range-for 直接迭代 Entity)。
    template <typename... Component>
    auto view() {
        return registry_.view<Component...>();
    }

    template <typename... Component>
    auto view() const {
        return registry_.view<Component...>();
    }

  private:
    entt::registry registry_;
};

// System: 约定统一签名 void(Registry&, float)。后续 Lua 规则 = 一个派发到 Lua 的 System 实现。
using System = std::function<void(Registry&, float)>;

// SystemManager: system 注册 + 统一更新循环。
class SystemManager {
  public:
    // 按注册顺序追加 (P0 不做运行时移除, 移除能力留后续需求)。
    void add(System system);

    // 统一更新循环: 按注册顺序依次调用每个 system。
    void update(Registry& registry, float dt);

  private:
    std::vector<System> systems_;
};
} // namespace ecs
