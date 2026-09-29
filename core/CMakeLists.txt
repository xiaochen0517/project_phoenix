add_executable(project_phoenix
    main.cpp
    log/app_log.cpp
    script/lua_runner.cpp
)

target_include_directories(project_phoenix PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(project_phoenix PRIVATE raylib rlimgui spdlog::spdlog ${LUA_LIBRARIES})
