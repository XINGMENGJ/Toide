cmake_minimum_required(VERSION 3.24)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

qt_standard_project_setup()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_library(toide_client_core STATIC
    src/app/main_window.cpp
    src/editor/editor_area_widget.cpp
    src/editor/editor_tab.cpp
    src/file_explorer/file_explorer_widget.cpp
    src/task_runner/task_config.cpp
    src/task_runner/task_diagnostic_parser.cpp
    src/task_runner/task_execution_request.cpp
    src/task_runner/task_process_runner.cpp
    src/task_runner/task_runner_widget.cpp
    src/workspace/recent_project_store.cpp
    src/workspace/workspace_manager.cpp
)

target_include_directories(toide_client_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(toide_client_core
    PUBLIC
        Qt6::Widgets
)

target_compile_features(toide_client_core PUBLIC cxx_std_20)
target_compile_definitions(toide_client_core
    PUBLIC
        TOIDE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

if(MSVC)
    target_compile_options(toide_client_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(toide_client_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

qt_add_executable(toide_client
    src/main.cpp
)

target_link_libraries(toide_client
    PRIVATE
        toide_client_core
)

function(add_toide_qt_test target)
    qt_add_executable(${target}
        ${ARGN}
    )

    target_link_libraries(${target}
        PRIVATE
            toide_client_core
            Qt6::Test
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
