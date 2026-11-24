cmake_minimum_required(VERSION 3.25)
project(rtos_examples VERSION 0.1.0 LANGUAGES C)

# The examples of Chapters 2, 3 and 5 of Volume 2 build for the Cortex-M4 of QEMU, with the toolchain file of
# the running project. They run with: ctest --preset m4  or  cmake --build --preset m4 --target run_<name>
get_filename_component(REPO "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH "${REPO}/cmake")
include(Firmware)

add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE -Wall -Wextra -Wshadow -Wconversion)

# A virtual clock that follows the instructions that run, not the wall clock (32 ns for each instruction, about a 31 MHz processor): the tick count and the
# times in the output are the same in every run. The idle task does not sleep, so the run is fast.
set(CMAKE_CROSSCOMPILING_EMULATOR
    qemu-system-arm -machine mps2-an386 -nographic -icount shift=5,sleep=off
    -semihosting-config enable=on,target=native -kernel)

enable_testing()
add_subdirectory("${REPO}/platform" platform)

# The kernel: six source files, the port for Cortex-M3 and Cortex-M4 without a floating point unit,
# and the heap implementation that merges freed blocks.
set(KERNEL "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel")
add_library(freertos STATIC
    "${KERNEL}/tasks.c" "${KERNEL}/queue.c" "${KERNEL}/list.c" "${KERNEL}/timers.c"
    "${KERNEL}/event_groups.c" "${KERNEL}/stream_buffer.c"
    "${KERNEL}/portable/GCC/ARM_CM3/port.c" "${KERNEL}/portable/MemMang/heap_4.c")
target_include_directories(freertos PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}" "${KERNEL}/include" "${KERNEL}/portable/GCC/ARM_CM3")
# The kernel is third party code: its warnings are not ours to fix.
target_compile_options(freertos PRIVATE -w)

add_library(rt STATIC src/rt.c)
target_include_directories(rt PUBLIC src)
target_link_libraries(rt PUBLIC freertos platform PRIVATE project_warnings)

# The kernel again, built with the configuration of config_x: static allocation, the trace facility,
# run time statistics and two notification indexes. The programs that need those options link
# freertos_x and rt_x instead of freertos and rt.
add_library(freertos_x STATIC
    "${KERNEL}/tasks.c" "${KERNEL}/queue.c" "${KERNEL}/list.c" "${KERNEL}/timers.c"
    "${KERNEL}/event_groups.c" "${KERNEL}/stream_buffer.c"
    "${KERNEL}/portable/GCC/ARM_CM3/port.c" "${KERNEL}/portable/MemMang/heap_4.c")
target_include_directories(freertos_x PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/config_x" "${KERNEL}/include" "${KERNEL}/portable/GCC/ARM_CM3")
target_compile_options(freertos_x PRIVATE -w)

add_library(rt_x STATIC src/rt.c)
target_include_directories(rt_x PUBLIC src)
target_link_libraries(rt_x PUBLIC freertos_x platform PRIVATE project_warnings)

# add_demo_with(<rt library> <name> <sources>...): one program, run by CTest in QEMU with a virtual clock.
function(add_demo_with rtlib name)
    add_executable(${name} ${ARGN} src/startup_rtos.c)
    target_link_libraries(${name} PRIVATE ${rtlib} firmware_link project_warnings)
    target_compile_options(${name} PRIVATE -fno-tree-loop-distribute-patterns)
    # The start-up code of these programs has more vectors than the one of the platform layer, so the
    # post build steps of firmware_finish() are written out here.
    target_link_options(${name} PRIVATE "-Wl,-Map=$<TARGET_FILE:${name}>.map")
    add_custom_command(TARGET ${name} POST_BUILD
        COMMAND arm-none-eabi-size $<TARGET_FILE:${name}> VERBATIM)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES TIMEOUT 60)
endfunction()

function(add_demo name)
    add_demo_with(rt ${name} ${ARGN})
endfunction()

function(add_demo_x name)
    # The memory of the idle task and the timer task is part of each program, so that the linker keeps it.
    add_demo_with(rt_x ${name} ${ARGN} src/static_mem.c)
endfunction()

add_demo(tasks demos/tasks.c)
add_demo(queue_isr demos/queue_isr.c)
add_demo(sync demos/sync.c)
add_demo(crit_sections demos/crit_sections.c)
add_demo(defer_isr demos/defer_isr.c)
add_demo_x(notify_flags demos/notify_flags.c)
add_demo(queue_patterns demos/queue_patterns.c)
add_demo(heap_stats demos/heap_stats.c)
add_demo(period_miss demos/period_miss.c)
add_demo_x(cpu_load demos/cpu_load.c)
add_demo(timer_timeout demos/timer_timeout.c)
add_demo(uart_stream demos/uart_stream.c)
add_demo_x(static_alloc demos/static_alloc.c)
add_demo(stackcheck demos/stackcheck.c)
add_demo(inversion_semaphore demos/inversion.c)
target_compile_definitions(inversion_semaphore PRIVATE USE_MUTEX=0)
add_demo(inversion_mutex demos/inversion.c)
target_compile_definitions(inversion_mutex PRIVATE USE_MUTEX=1)
add_demo(preempt demos/preempt.c)
add_demo(periodic demos/periodic.c)
add_demo(latency demos/latency.c)

# The same jobs on the bare processor, without the kernel: only the platform layer and the start-up code.
add_executable(superloop demos/superloop.c src/startup_rtos.c)
target_link_libraries(superloop PRIVATE platform firmware_link project_warnings)
target_compile_options(superloop PRIVATE -fno-tree-loop-distribute-patterns)
target_link_options(superloop PRIVATE "-Wl,-Map=$<TARGET_FILE:superloop>.map")
add_custom_command(TARGET superloop POST_BUILD COMMAND arm-none-eabi-size $<TARGET_FILE:superloop> VERBATIM)
add_test(NAME superloop COMMAND superloop)
set_tests_properties(superloop PROPERTIES TIMEOUT 60)
add_executable(bare_latency demos/bare_latency.c src/startup_rtos.c)
target_link_libraries(bare_latency PRIVATE platform firmware_link project_warnings)
target_compile_options(bare_latency PRIVATE -fno-tree-loop-distribute-patterns)
target_link_options(bare_latency PRIVATE "-Wl,-Map=$<TARGET_FILE:bare_latency>.map")
add_custom_command(TARGET bare_latency POST_BUILD COMMAND arm-none-eabi-size $<TARGET_FILE:bare_latency> VERBATIM)
add_test(NAME bare_latency COMMAND bare_latency)
set_tests_properties(bare_latency PROPERTIES TIMEOUT 60)

# This program ends on purpose with the stack overflow hook, which exits with status 4.
set_tests_properties(stackcheck PROPERTIES PASS_REGULAR_EXPRESSION "stack overflow in task: hungry")
