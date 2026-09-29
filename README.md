# project_phoenix

基于 [raylib](https://www.raylib.com/) 的 C++17 桌面程序, 集成 spdlog 日志和 Lua 脚本。

## 依赖

| 库 | 来源 | 说明 |
|---|---|---|
| raylib 6.0 | CMake `FetchContent` (源码构建) | 固定 URL 和 SHA256, 使用 raylib 自带的 GLFW |
| Dear ImGui 1.92.7 | CMake `FetchContent` (源码构建) | 固定 URL 和 SHA256 |
| rlImGui (`Raylib_6_0` tag) | CMake `FetchContent` (源码构建) | raylib 的 ImGui 后端, 与 ImGui 一起编译为静态库 `rlimgui` |
| spdlog | vcpkg (manifest 模式) | 版本由 `vcpkg.json` 的 `builtin-baseline` 固定 |
| Lua | vcpkg (manifest 模式) | 同上 |

> raylib 不通过 vcpkg 获取: vcpkg 版本使用外部 glfw3, 在开发环境下出现过首帧后窗口无响应的问题。

## 环境要求

- CMake >= 3.21, Ninja
- 已安装 vcpkg, 并设置环境变量 `VCPKG_ROOT`
- Windows: Visual Studio (MSVC), 仅支持 MSVC 工具链
- Linux: GCC 或 Clang, 以及 raylib 所需的开发包 (Debian/Ubuntu 示例):

  ```
  sudo apt install build-essential ninja-build pkg-config curl zip unzip tar \
      libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev libgl1-mesa-dev
  ```

## 构建

可用的 preset: `msvc-debug`, `msvc-release` (Windows), `linux-debug`, `linux-release` (Linux)。

```
cmake --preset msvc-release
cmake --build --preset msvc-release
```

Windows 命令行需要在 "x64 Native Tools Command Prompt for VS" 中执行, 或先运行 `vcvars64.bat`。
首次 configure 会安装 vcpkg 依赖并从源码编译 raylib, 耗时约 1 到 2 分钟。

### CLion

打开项目后启用对应 preset 的 profile。Windows 使用 `Visual Studio` toolchain, 不要使用 MinGW。

## 目录结构

```
CMakeLists.txt        顶层: 依赖获取 (vcpkg / FetchContent), 构建 rlimgui 静态库
CMakePresets.json     构建 preset
vcpkg.json            vcpkg 依赖清单
core/
  CMakeLists.txt      可执行目标
  main.cpp            程序入口
  log/                spdlog 封装
  script/             Lua 封装
```

`log/` 和 `script/` 各自封装第三方库, 避免 spdlog 引入的 `<windows.h>` 与 `raylib.h` 在同一个源文件中冲突。
