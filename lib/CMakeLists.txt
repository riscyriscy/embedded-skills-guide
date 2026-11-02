set(VERSION_C "${CMAKE_CURRENT_BINARY_DIR}/version.c")

add_custom_command(
    OUTPUT "${VERSION_C}"
    COMMAND ${CMAKE_COMMAND}
            -DSRC=${CMAKE_CURRENT_SOURCE_DIR}/src/version.c.in
            -DDST=${VERSION_C}
            -DPROJECT_VERSION=${PROJECT_VERSION}
            -DGIT_DIR=${PROJECT_SOURCE_DIR}
            -P ${PROJECT_SOURCE_DIR}/cmake/GenVersion.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/version.c.in
            ${PROJECT_SOURCE_DIR}/cmake/GenVersion.cmake
    COMMENT "Generating version.c")
add_custom_target(sensor_version_gen DEPENDS "${VERSION_C}")

add_library(sensor STATIC src/ringbuf.c src/stats.c "${VERSION_C}")
add_dependencies(sensor sensor_version_gen)

target_include_directories(sensor PUBLIC include)
target_compile_definitions(sensor PUBLIC RINGBUF_CAPACITY=16u)
target_link_libraries(sensor PRIVATE project_warnings)
