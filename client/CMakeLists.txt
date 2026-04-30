cmake_minimum_required(VERSION 3.24)

find_package(Qt6 REQUIRED COMPONENTS Network Widgets Test OPTIONAL_COMPONENTS WebSockets)

qt_standard_project_setup()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(TOIDE_CLIENT_CORE_SOURCES
    src/app/main_window.cpp
    src/collaboration/collaboration_panel_widget.cpp
    src/editor/editor_area_widget.cpp
    src/editor/editor_tab.cpp
    src/file_explorer/file_explorer_widget.cpp
    src/git/git_status_widget.cpp
    src/network/network_client.cpp
    src/settings/server_endpoint_settings.cpp
    src/task_runner/task_config.cpp
    src/task_runner/task_diagnostic_parser.cpp
    src/task_runner/task_execution_request.cpp
    src/task_runner/task_process_runner.cpp
    src/task_runner/task_runner_widget.cpp
    src/workspace/recent_project_store.cpp
    src/workspace/workspace_compile_widget.cpp
    src/workspace/workspace_manager.cpp
)

if(TARGET Qt6::WebSockets)
    list(APPEND TOIDE_CLIENT_CORE_SOURCES src/network/collaboration_websocket_client.cpp)
    message(STATUS "Qt WebSockets: enabled (CollaborationWebSocketClient will be built)")
else()
    message(STATUS "Qt WebSockets: not installed — skipping CollaborationWebSocketClient; add via Qt Maintenance Tool to enable")
endif()

add_library(toide_client_core STATIC
    ${TOIDE_CLIENT_CORE_SOURCES}
)

target_include_directories(toide_client_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(toide_client_core
    PUBLIC
        Qt6::Network
        Qt6::Widgets
)

if(TARGET Qt6::WebSockets)
    target_link_libraries(toide_client_core PUBLIC Qt6::WebSockets)
    target_compile_definitions(toide_client_core PUBLIC TOIDE_HAVE_QT_WEBSOCKETS=1)
endif()

target_compile_features(toide_client_core PUBLIC cxx_std_20)
target_compile_definitions(toide_client_core
    PUBLIC
        TOIDE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    get_filename_component(_TOIDE_CXX_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    file(TO_CMAKE_PATH "${_TOIDE_CXX_BIN_DIR}" _TOIDE_CXX_BIN_DIR)
    string(REPLACE "\\" "/" _TOIDE_CXX_BIN_DIR_SLASH "${_TOIDE_CXX_BIN_DIR}")
    target_compile_definitions(toide_client_core
        PUBLIC
            "TOIDE_BUILD_CXX_BIN_DIR=\"${_TOIDE_CXX_BIN_DIR_SLASH}\"")
endif()

if(MSVC)
    # Qt headers require MSVC to report a standards-conforming __cplusplus value.
    target_compile_options(toide_client_core PUBLIC /Zc:__cplusplus)
    target_compile_options(toide_client_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(toide_client_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

qt_add_executable(toide_client
    src/main.cpp
)

qt_add_resources(toide_client toide_changelog
    PREFIX "/toide"
    FILES
        resources/app_changelog.txt
)

target_link_libraries(toide_client
    PRIVATE
        toide_client_core
        Qt6::Widgets
)

function(add_toide_qt_test target)
    qt_add_executable(${target}
        ${ARGN}
    )

    target_link_libraries(${target}
        PRIVATE
            toide_client_core
            Qt6::Test
            Qt6::Widgets
    )

    add_test(NAME ${target} COMMAND ${target})
endfunction()

enable_testing()

add_toide_qt_test(toide_main_window_smoke_test
    tests/main_window_smoke_test.cpp
)

add_toide_qt_test(toide_workspace_manager_test
    tests/workspace_manager_test.cpp
)

add_toide_qt_test(toide_editor_tab_test
    tests/editor_tab_test.cpp
)

add_toide_qt_test(toide_editor_area_widget_test
    tests/editor_area_widget_test.cpp
)

add_toide_qt_test(toide_recent_project_store_test
    tests/recent_project_store_test.cpp
)

add_toide_qt_test(toide_git_status_widget_test
    tests/git_status_widget_test.cpp
)

add_toide_qt_test(toide_file_explorer_widget_test
    tests/file_explorer_widget_test.cpp
)

add_toide_qt_test(toide_network_client_test
    tests/network_client_test.cpp
)

if(TARGET Qt6::WebSockets)
    add_toide_qt_test(toide_collaboration_websocket_client_test
        tests/collaboration_websocket_client_test.cpp
    )
endif()

add_toide_qt_test(toide_collaboration_panel_widget_test
    tests/collaboration_panel_widget_test.cpp
)

add_toide_qt_test(toide_server_endpoint_settings_test
    tests/server_endpoint_settings_test.cpp
)

add_toide_qt_test(toide_task_config_test
    tests/task_config_test.cpp
)

add_toide_qt_test(toide_task_diagnostic_parser_test
    tests/task_diagnostic_parser_test.cpp
)

add_toide_qt_test(toide_task_execution_request_test
    tests/task_execution_request_test.cpp
)

add_toide_qt_test(toide_task_process_runner_test
    tests/task_process_runner_test.cpp
)

add_toide_qt_test(toide_task_runner_widget_test
    tests/task_runner_widget_test.cpp
)
