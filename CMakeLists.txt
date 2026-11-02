cmake_minimum_required(VERSION 3.25)
project(sensorfw VERSION 0.1.0 LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(Firmware)

# Warnings are a property that targets opt into, not a global switch.
add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE -Wall -Wextra -Wshadow -Wconversion)

enable_testing()
add_subdirectory(lib)
add_subdirectory(platform)
add_subdirectory(app)
add_subdirectory(test)
