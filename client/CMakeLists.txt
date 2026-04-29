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
