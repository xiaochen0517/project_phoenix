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
