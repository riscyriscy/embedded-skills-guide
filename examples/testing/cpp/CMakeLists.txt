cmake_minimum_required(VERSION 3.25)
project(sensor_cpp_tests VERSION 0.1.0 LANGUAGES CXX)

# GoogleTest comes from the libgtest-dev package. The tests run on the host only.
find_package(GTest CONFIG REQUIRED)
enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The class under test is the ring buffer template of Chapter 6 of Volume 1. It is a header, so the test
# program compiles it with the warning options of this project.
add_executable(test_ringbuf_cpp tests/test_ringbuf_cpp.cpp)
# File names in messages stay the same in any directory, as in the C test project.
target_compile_options(test_ringbuf_cpp PRIVATE "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")
target_include_directories(test_ringbuf_cpp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../modern-cpp")
target_compile_options(test_ringbuf_cpp PRIVATE -Wall -Wextra -Wshadow -Wconversion)
target_link_libraries(test_ringbuf_cpp PRIVATE GTest::gtest GTest::gtest_main)

# One CTest entry for each test, found by running the program with --gtest_list_tests.
include(GoogleTest)
gtest_discover_tests(test_ringbuf_cpp)
