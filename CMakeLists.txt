cmake_minimum_required(VERSION 3.20)
project(timeseries-expr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TSEXPR_BUILD_TESTS "Build tests" ON)
option(TSEXPR_BUILD_EXAMPLES "Build examples" ON)

add_library(tsexpr
  src/lexer.cpp
  src/parser.cpp
  src/program.cpp
)
target_include_directories(tsexpr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(tsexpr PUBLIC cxx_std_17)

if (TSEXPR_BUILD_EXAMPLES)
  add_executable(tsexpr_toy examples/toy_backend.cpp)
  target_link_libraries(tsexpr_toy PRIVATE tsexpr)
endif()

if (TSEXPR_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  add_executable(tsexpr_tests tests/test_expr.cpp)
  target_link_libraries(tsexpr_tests PRIVATE tsexpr GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(tsexpr_tests)
endif()
