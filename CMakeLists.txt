cmake_minimum_required(VERSION 3.21)
project(project_phoenix C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# vcpkg 在 project() 内 (编译器检测之前) 就决定 triplet, 无法自动匹配 MinGW,
# 而 preset 中的 triplet 是 MSVC ABI, 因此 Windows 上仅支持 MSVC。
if(WIN32 AND NOT MSVC)
    message(FATAL_ERROR "Windows 仅支持 MSVC 工具链, 请使用 msvc-* preset。")
endif()

# spdlog / Lua 由 vcpkg (manifest 模式) 提供。
find_package(spdlog CONFIG REQUIRED)
find_package(Lua REQUIRED)

# raylib 与项目使用同一个 compiler 从源码构建, 并使用其自带的 GLFW。
# 不要改用 vcpkg 的 raylib: 它使用外部 glfw3 (USE_EXTERNAL_GLFW=ON),
# 在本项目环境下会导致首帧后 swap 忙等, 窗口无响应。
include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(USE_EXTERNAL_GLFW OFF CACHE STRING "" FORCE)

FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz
    URL_HASH SHA256=2B3EE1E2120C7A0796B33062C7E9A694DD8A8CAA56A96319AC8C8ECF54A90D0B
)
FetchContent_MakeAvailable(raylib)

# Dear ImGui 与 rlImGui 均不提供 CMakeLists, 这里只下载源码并自行构建静态库。
# rlImGui 使用 Raylib_6_0 tag 对应的提交, 该 tag 与 raylib 6.0 / ImGui 1.92.7 配套。
FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.92.7.tar.gz
    URL_HASH SHA256=B21B14CE1EF6DD4D85FA54F68F8449A9BE94F3B453ABA7FE4EA2A9764E43A5EF
)
FetchContent_Declare(
    rlimgui
    URL https://github.com/raylib-extras/rlImGui/archive/3bc5731c4216bb8caa67fbea24aa85ce80d57ccb.tar.gz
    URL_HASH SHA256=2198C4EB4C0A2B8EFC2204DA046EF014A2C7B736E1483039BADEE6D19A7ED35A
)
FetchContent_MakeAvailable(imgui rlimgui)

add_library(rlimgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp
)
target_include_directories(rlimgui PUBLIC ${imgui_SOURCE_DIR} ${rlimgui_SOURCE_DIR})
target_link_libraries(rlimgui PUBLIC raylib)

add_subdirectory(core)
