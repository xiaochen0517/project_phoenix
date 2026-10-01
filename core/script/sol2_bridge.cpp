#include "script/sol2_bridge.h"

#include <sol/sol.hpp>

#include <sstream>
#include <string>
#include <vector>

namespace sol2_bridge {
    namespace {
        // 用于 usertype 测试的简单类: 多构造 + 成员函数 + 成员变量。
        struct Counter {
            int value = 0;

            Counter() = default;

            Counter(int v) : value(v) {
            }

            void add(int n) { value += n; }
            int get() const { return value; }
        };
    } // namespace

    std::string self_test() {
        sol::state lua;
        // 注意: 必须用无参 open_libraries() (等价 luaL_openlibs), 才能把 jit 注册为全局变量。
        // 带参重载对 lib::jit 用 luaL_requiref(L, "jit", luaopen_jit, 0) 仅写入 package.loaded,
        // 不会创建全局 jit, 导致 jit.version 为 nil 而崩溃。
        lua.open_libraries();

        std::ostringstream out;
        int passed = 0;
        int failed = 0;

        auto check = [&](const char *name, bool ok, const std::string &detail = {}) {
            out << "  [" << (ok ? "PASS" : "FAIL") << "] " << name;
            if (!detail.empty()) {
                out << "  (" << detail << ")";
            }
            out << '\n';
            ok ? ++passed : ++failed;
        };

        // 1. JIT 经 sol2 可用 (版本号非空 + JIT 开启)
        {
            const std::string version = lua.script("return jit.version").get<std::string>();
            const bool jitOn = lua.script("return jit.status()").get<bool>();
            check("JIT available via sol2", !version.empty() && jitOn, version);
        }

        // 2. C++ 函数 -> Lua 调用
        {
            lua.set_function("add", [](int a, int b) { return a + b; });
            const int r = lua.script("return add(6, 7)").get<int>();
            check("C++ function called from Lua", r == 13);
        }

        // 3. Lua 函数 -> C++ 调用 (多返回值)
        {
            lua.script("function mul_add(a, b) return a * b, a + b end");
            sol::protected_function f = lua["mul_add"];
            sol::protected_function_result res = f(6, 7);
            const int prod = res.get<int>(0);
            const int sum = res.get<int>(1);
            check("Lua function called from C++ (multi-return)", prod == 42 && sum == 13);
        }

        // 4. usertype 绑定 C++ 类 (多构造 + 方法 + 成员变量)
        {
            lua.new_usertype<Counter>(
                "Counter",
                sol::constructors<Counter(), Counter(int)>(),
                "add", &Counter::add,
                "get", &Counter::get,
                "value", &Counter::value);

            const int v = lua.script(
                "local c = Counter.new(10)\n"
                "c:add(5)\n"
                "c.value = c.value + 1\n"
                "return c:get()").get<int>();
            check("usertype bind + methods + member var", v == 16);
        }

        // 5. 容器 std::vector<int> 往返 (table <-> vector)
        // 注意: 容器参数必须按值传 (std::vector<int>), 或用 sol::as_table_t 包装;
        // 若用 const std::vector<int>& (引用), sol2 会走 usertype 取值路径,
        // 对 table 调 lua_touserdata 得空指针而崩溃。
        {
            lua.set_function("sum_vec", [](std::vector<int> v) {
                int s = 0;
                for (int x: v) {
                    s += x;
                }
                return s;
            });
            lua.set_function("range", [](int n) {
                std::vector<int> v;
                for (int i = 1; i <= n; ++i) {
                    v.push_back(i);
                }
                return v;
            });
            const int sum = lua.script("return sum_vec({1, 2, 3, 4})").get<int>();
            const int len = lua.script("return #range(5)").get<int>();
            check("std::vector<int> round-trip", sum == 10 && len == 5);
        }

        // 6. 异常处理 (Lua 错误经 pcall 捕获, 不抛 C++ 异常)
        {
            sol::protected_function_result res = lua.script("error('boom')", sol::script_pass_on_error);
            check("Lua error captured (no throw)", !res.valid());
        }

        out << "sol2 + LuaJIT self-test: " << passed << " passed, " << failed << " failed";
        return out.str();
    }
} // namespace sol2_bridge
