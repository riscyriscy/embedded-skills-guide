cmake_minimum_required(VERSION 3.25)
project(sensor_tests VERSION 0.1.0 LANGUAGES C)

# The tests build the libraries of the sensorfw project from their source files.
get_filename_component(REPO "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH "${REPO}/cmake")
include(Firmware)

add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE -Wall -Wextra -Wshadow -Wconversion)

# Instrumentation applies to everything that is built after these lines: the code under test too.
set(SANITIZE "" CACHE STRING "Sanitizers for the host build, for example address,undefined")
option(COVERAGE "Instrument the host build for gcov" OFF)
option(FUZZ "Build the libFuzzer target (needs clang)" OFF)
option(FUZZ_KEEP_CRC "Keep the check value test in the fuzzing build" OFF)
if(SANITIZE)
    add_compile_options(-fsanitize=${SANITIZE} -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=${SANITIZE})
endif()
if(COVERAGE)
    add_compile_options(--coverage -O0)
    add_link_options(--coverage)
endif()
if(FUZZ)
    add_compile_options(-fsanitize=fuzzer-no-link)
    if(NOT FUZZ_KEEP_CRC)
        add_compile_definitions(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
    endif()
endif()

option(PATH_MAP "Replace the source directory in messages and debug info of host builds" ON)
if(NOT CMAKE_CROSSCOMPILING AND PATH_MAP)
    # The toolchain file does this for the target: file names in messages stay the same in any
    # directory. Coverage tools must open the sources again, so coverage presets turn it off.
    add_compile_options("-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")
endif()

enable_testing()
add_subdirectory("${REPO}/platform" platform)

# The code under test: the sources of the library, compiled with the options of this project.
add_library(sensor STATIC "${REPO}/lib/src/ringbuf.c" "${REPO}/lib/src/stats.c")
target_include_directories(sensor PUBLIC "${REPO}/lib/include")
target_compile_definitions(sensor PUBLIC RINGBUF_CAPACITY=16u)
add_library(filters STATIC "${REPO}/lib/filters/src/median.c")
target_include_directories(filters PUBLIC "${REPO}/lib/filters/include")
target_link_libraries(filters PUBLIC sensor)

# Unity, in the form of three source files. The target needs a function that writes one character.
set(TEST_OUTPUT semihost CACHE STRING "Output of the tests on the target: semihost or uart")
add_library(unity STATIC unity/unity.c)
target_include_directories(unity PUBLIC unity)
if(CMAKE_CROSSCOMPILING)
    add_library(unity_output STATIC target/output_${TEST_OUTPUT}.c)
    target_include_directories(unity_output PUBLIC target)
    target_compile_definitions(unity PUBLIC UNITY_INCLUDE_CONFIG_H)
    target_include_directories(unity PUBLIC target)
    target_link_libraries(unity PUBLIC unity_output)
endif()

add_library(frame STATIC frame/frame.c)
target_include_directories(frame PUBLIC frame)
target_link_libraries(frame PRIVATE project_warnings)

add_library(sampler STATIC sampler/sampler.c)
target_include_directories(sampler PUBLIC sampler)
target_link_libraries(sampler PUBLIC sensor PRIVATE project_warnings)

# add_unity_test(<name> <sources>...) builds one test program and registers it with CTest.
function(add_unity_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE unity platform firmware_link project_warnings)
    if(CMAKE_CROSSCOMPILING)
        # A source of the program itself is always linked; an archive member is taken only when it
        # is needed at the moment that the linker reads the archive.
        target_sources(${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/target/unwind_stub.c")
    endif()
    firmware_finish(${name})
    add_test(NAME ${name} COMMAND ${name})
    # A test that hangs must not block the run for ever. The default of CTest is 25 minutes.
    set_tests_properties(${name} PROPERTIES TIMEOUT 30)
endfunction()

add_unity_test(test_ringbuf tests/test_ringbuf.c)
target_link_libraries(test_ringbuf PRIVATE filters sensor)

add_unity_test(test_frame tests/test_frame.c)
target_link_libraries(test_frame PRIVATE frame)

add_unity_test(test_sampler tests/test_sampler.c tests/fake_adc.c)
target_link_libraries(test_sampler PRIVATE sampler)

if(NOT CMAKE_CROSSCOMPILING)
    # cmocka needs a hosted C library, so its tests build for the host only.
    find_package(cmocka CONFIG)
    if(cmocka_FOUND)
        add_executable(test_sampler_cmocka tests/test_sampler_cmocka.c)
        target_link_libraries(test_sampler_cmocka PRIVATE sampler cmocka::cmocka project_warnings)
        add_test(NAME test_sampler_cmocka COMMAND test_sampler_cmocka)
    endif()
endif()

if(FUZZ)
    add_executable(fuzz_frame fuzz/fuzz_frame.c)
    target_link_libraries(fuzz_frame PRIVATE frame)
    target_link_options(fuzz_frame PRIVATE -fsanitize=fuzzer)
endif()
