#!/bin/bash
# Runs a built example in QEMU with a virtual clock of 32 ns per instruction, the clock of the FreeRTOS
# examples. The program ends with sys_reboot, which -no-reboot turns into an exit of QEMU.
# Usage: examples/zephyr/run.sh <app>
set -eu
app=${1:?usage: run.sh <app>}
here=$(cd "$(dirname "$0")" && pwd)
build=${BUILD_DIR:-$here/../../build/zephyr-$app}
timeout "${RUN_TIMEOUT:-30}" qemu-system-arm -cpu cortex-m4 -machine mps2-an386 -nographic \
    -icount shift=5,sleep=off -no-reboot -net none -serial mon:stdio \
    -kernel "$build/zephyr/zephyr.elf" < /dev/null 2>&1 | grep -v "has no peer"
