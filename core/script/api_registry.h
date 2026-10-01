#pragma once

struct lua_State; // 前置声明, 不 include lua.h / sol2 (ADR-5)

namespace lua_engine {
class Engine;
}

namespace event {
class Bus;
}

namespace api {
// 汇总注册所有 api.* 模块, 把 C++ 能力暴露给 Lua 侧 (api.<module>.<func> 命名空间)。
void register_all(lua_engine::Engine& engine);

// 注册 api.event.* (事件总线绑定)。事件总线为有状态对象, 需外部注入 bus 实例。
void register_event(lua_State* L, event::Bus& bus);
} // namespace api
