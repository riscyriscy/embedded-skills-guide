#!/bin/bash
# Builds one RTOS demo with several optimization options, runs it in QEMU and prints the sizes.
# Usage: examples/size/variants.sh [demo]    (the demo is a target of examples/freertos, default: tasks)
set -eu
demo=${1:-tasks}
root=$(cd "$(dirname "$0")/../.." && pwd)
printf '%-14s %7s %6s %6s  %s\n' options text data bss run
for flags in "-O0" "-Og" "-O1" "-O2" "-O3" "-Os" "-Os -flto"; do
    dir=$root/build/size/$(echo "$flags" | tr -c 'A-Za-z0-9\n' '_')
    cmake -S "$root/examples/freertos" -B "$dir" -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$root/cmake/arm-none-eabi.cmake" -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_FLAGS_RELEASE="$flags -DNDEBUG" > /dev/null
    cmake --build "$dir" --target "$demo" > /dev/null
    if ctest --test-dir "$dir" -R "^$demo\$" > /dev/null 2>&1; then run=pass; else run=FAIL; fi
    arm-none-eabi-size "$dir/$demo" | awk -v f="$flags" -v r="$run" 'NR == 2 { printf "%-14s %7d %6d %6d  %s\n", f, $1, $2, $3, r }'
done
