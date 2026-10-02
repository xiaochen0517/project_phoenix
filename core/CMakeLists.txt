add_executable(project_phoenix
    main.cpp
    app/app.cpp
    app/scene_host.cpp
    scene/debug/model_viewer_game.cpp
    scene/main_game/demo_scene.cpp
    util/asset_path.cpp
    log/app_log.cpp
    script/lua_engine.cpp
    script/api_registry.cpp
    event/event_bus.cpp
    config/config.cpp
    ecs/ecs.cpp
    camera/camera.cpp
    input/input.cpp
    model/model_loader.cpp
)

target_include_directories(project_phoenix PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LUAJIT_INCLUDE_DIR}
)

target_link_libraries(project_phoenix PRIVATE raylib rlimgui spdlog::spdlog ${LUAJIT_LIBRARY} sol2::sol2 nlohmann_json::nlohmann_json
    EnTT::EnTT)

# 单元测试目标: 无头运行, 不链接 raylib / rlimgui / model_loader (后者需 GL 上下文与资产)。
# 仅复用可自动测试的纯逻辑模块 (log / script), 链接 Catch2 提供的 main。
add_executable(project_phoenix_tests
    log/app_log.cpp
    script/lua_engine.cpp
    script/api_registry.cpp
    event/event_bus.cpp
    config/config.cpp
    ecs/ecs.cpp
    camera/camera.cpp
    input/input.cpp
    tests/test_lua_engine.cpp
    tests/test_api_bindings.cpp
    tests/test_event_bus.cpp
    tests/test_config.cpp
    tests/test_ecs.cpp
    tests/test_camera.cpp
    tests/test_api_camera.cpp
    tests/test_input.cpp
    tests/test_app_log.cpp
    tests/success_listener.cpp
)

target_include_directories(project_phoenix_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LUAJIT_INCLUDE_DIR}
)

# 无头测试不能依赖 raylib 的 FileExists/GetApplicationDirectory 定位脚本目录,
# 故将项目根目录下的 lua/test 与 config/test 绝对路径注入测试目标, 测试据此拼接脚本完整路径。
target_compile_definitions(project_phoenix_tests PRIVATE
    PROJECT_PHOENIX_LUA_DIR="${CMAKE_SOURCE_DIR}/lua/test"
    PROJECT_PHOENIX_CONFIG_DIR="${CMAKE_SOURCE_DIR}/config/test"
)

target_link_libraries(project_phoenix_tests PRIVATE Catch2::Catch2WithMain spdlog::spdlog ${LUAJIT_LIBRARY} sol2::sol2
    nlohmann_json::nlohmann_json EnTT::EnTT)

add_test(NAME project_phoenix_tests COMMAND project_phoenix_tests)
