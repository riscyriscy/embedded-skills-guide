set(VERSION_C "${CMAKE_CURRENT_BINARY_DIR}/version.c")

# A custom target has no output file to compare, so it runs on every build. GenVersion.cmake
# rewrites version.c only when the text changes, so an unchanged version recompiles nothing.
add_custom_target(sensor_version_gen
    COMMAND ${CMAKE_COMMAND}
            -DSRC=${CMAKE_CURRENT_SOURCE_DIR}/src/version.c.in
            -DDST=${VERSION_C}
            -DPROJECT_VERSION=${PROJECT_VERSION}
            -DGIT_DIR=${PROJECT_SOURCE_DIR}
            -P ${PROJECT_SOURCE_DIR}/cmake/GenVersion.cmake
    BYPRODUCTS "${VERSION_C}"
    COMMENT "Generating version.c"
    VERBATIM)

add_library(sensor STATIC src/ringbuf.c src/stats.c "${VERSION_C}")
add_dependencies(sensor sensor_version_gen)

target_include_directories(sensor PUBLIC include)
target_compile_definitions(sensor PUBLIC RINGBUF_CAPACITY=16u)
target_link_libraries(sensor PRIVATE project_warnings)
