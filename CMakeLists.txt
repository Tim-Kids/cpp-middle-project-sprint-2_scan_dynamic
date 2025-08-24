cmake_minimum_required(VERSION 3.25)
project(scan VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

option(SCAN_BUILD_TESTS "Build tests" ON)

set(target scan)

add_library(${target} INTERFACE)

target_include_directories(${target} INTERFACE include/)

if(SCAN_BUILD_TESTS)
    # GoogleTest via FetchContent ensures reproducible builds without system packages
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    )
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    set(test_target scan_tests)
    add_executable(${test_target} tests/main.cpp tests/scan_test.cpp)
    target_link_libraries(${test_target} PRIVATE ${target} GTest::gtest GTest::gtest_main)

    # Включаем тестирование
    enable_testing()
    add_test(NAME ${test_target} COMMAND ${test_target})
endif()