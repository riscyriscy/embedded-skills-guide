#!/bin/bash
# Builds trace_demo for the Cortex-M4 and runs it in QEMU with a virtual clock (32 ns
# for each instruction), so that the timestamps are the same in every run. The raw
# trace goes to standard output.
# Usage: examples/trace/run.sh
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cpu=(-mcpu=cortex-m4 -mthumb -mfloat-abi=soft -ffunction-sections -fdata-sections)
arm-none-eabi-gcc "${cpu[@]}" -O1 -Wall -Wextra -Wconversion \
    -I"$root/platform/include" -I"$here" \
    "$here/trace_demo.c" "$here/trace.c" \
    "$root/examples/practice/startup_irq.c" \
    "$root/platform/arm-m4/platform_semihost.c" \
    --specs=nano.specs -nostartfiles -Wl,--gc-sections \
    -T"$root/platform/arm-m4/link.ld" -o "$work/trace.elf"
qemu-system-arm -machine mps2-an386 -nographic -icount shift=5,sleep=off \
    -semihosting-config enable=on,target=native -kernel "$work/trace.elf" \
    < /dev/null 2>&1
