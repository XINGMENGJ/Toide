cmake_minimum_required(VERSION 3.24)

add_library(toide_server_core STATIC
    src/health/health_response.cpp
)

target_include_directories(toide_server_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(toide_server_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(toide_server_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(toide_server_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

find_package(Drogon CONFIG QUIET)

if(Drogon_FOUND)
    add_executable(toide_server
        src/main.cpp
        src/api/collaboration_ws_controller.cpp
        src/api/health_controller.cpp
    )

    target_link_libraries(toide_server
        PRIVATE
            toide_server_core
            Drogon::Drogon
    )

    if(MSVC)
        target_compile_options(toide_server PRIVATE /W4 /permissive-)
    else()
        target_compile_options(toide_server PRIVATE -Wall -Wextra -Wpedantic)
    endif()
else()
    message(STATUS "Drogon not found; building toide_server_core and tests only. Install Drogon to build toide_server.")
endif()

add_executable(toide_server_health_response_test
    tests/health_response_test.cpp
)

target_link_libraries(toide_server_health_response_test
    PRIVATE
        toide_server_core
)

add_test(NAME toide_server_health_response_test COMMAND toide_server_health_response_test)
