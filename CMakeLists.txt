cmake_minimum_required(VERSION 3.20)
project(timeseries-expr VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TSEXPR_BUILD_TESTS "Build tests" ON)

add_library(tsexpr
    src/expr.cpp
    src/timeseries_stub.cpp
)
target_include_directories(tsexpr
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(tsexpr PUBLIC cxx_std_17)

# Warnings (portable-ish)
if (MSVC)
    target_compile_options(tsexpr PRIVATE /W4 /permissive-)
else()
    target_compile_options(tsexpr PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (TSEXPR_BUILD_TESTS)
    include(CTest)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    # For MSVC: avoid overriding /MDd etc.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(tsexpr_tests
        tests/test_expr.cpp
    )
    target_link_libraries(tsexpr_tests PRIVATE tsexpr GTest::gtest_main)
    target_compile_features(tsexpr_tests PRIVATE cxx_std_17)

    include(GoogleTest)
    gtest_discover_tests(tsexpr_tests)
endif()
