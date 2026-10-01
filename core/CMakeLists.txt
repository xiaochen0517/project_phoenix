add_executable(project_phoenix
    main.cpp
    log/app_log.cpp
    script/lua_runner.cpp
    script/sol2_bridge.cpp
    model/model_loader.cpp
)

target_include_directories(project_phoenix PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LUAJIT_INCLUDE_DIR}
)

target_link_libraries(project_phoenix PRIVATE raylib rlimgui spdlog::spdlog ${LUAJIT_LIBRARY} sol2::sol2)

# 单元测试目标: 无头运行, 不链接 raylib / rlimgui / model_loader (后者需 GL 上下文与资产)。
# 仅复用可自动测试的纯逻辑模块 (log / script), 链接 Catch2 提供的 main。
add_executable(project_phoenix_tests
    log/app_log.cpp
    script/lua_runner.cpp
    script/sol2_bridge.cpp
    tests/test_lua_runner.cpp
    tests/test_sol2_bridge.cpp
    tests/test_app_log.cpp
)

target_include_directories(project_phoenix_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LUAJIT_INCLUDE_DIR}
)

target_link_libraries(project_phoenix_tests PRIVATE Catch2::Catch2WithMain spdlog::spdlog ${LUAJIT_LIBRARY} sol2::sol2)

add_test(NAME project_phoenix_tests COMMAND project_phoenix_tests)
