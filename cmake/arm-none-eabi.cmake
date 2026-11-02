set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Look for programs on the host, but for libraries and headers only in the target sysroot.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=soft")
set(CMAKE_C_FLAGS_INIT "${CPU_FLAGS} -ffunction-sections -fdata-sections")
# Per-configuration flags are set in the cache. CMAKE_C_FLAGS_RELEASE_INIT would be
# followed by the compiler default "-O3 -DNDEBUG", and the last -O option wins.
set(CMAKE_C_FLAGS_DEBUG "-Og -g3" CACHE STRING "")
set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG" CACHE STRING "")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CPU_FLAGS} -Wl,--gc-sections --specs=nano.specs -nostartfiles")

# ctest runs each test executable through this command line, with the executable appended.
set(CMAKE_CROSSCOMPILING_EMULATOR
    qemu-system-arm -machine mps2-an386 -nographic
    -semihosting-config enable=on,target=native -kernel)
