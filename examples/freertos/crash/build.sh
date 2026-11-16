#!/bin/bash
# Builds the bootloader and the application, and runs both in QEMU.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
ex=$here/..
root=$ex/../..
out=${OUT:-$here/out}
mkdir -p "$out"
cpu=(-mcpu=cortex-m4 -mthumb -mfloat-abi=soft -ffunction-sections -fdata-sections -g -O1 -Wall -Wextra
     -fno-tree-loop-distribute-patterns)
k=$ex/FreeRTOS-Kernel
inc=(-I"$here" -I"$ex/src" -I"$root/examples/watchdog" -I"$k/include" -I"$k/portable/GCC/ARM_CM3"
     -I"$root/platform/include")
link=(--specs=nano.specs -nostartfiles -Wl,--gc-sections -L"$here")

# the bootloader
arm-none-eabi-gcc "${cpu[@]}" "${inc[@]}" "${link[@]}" -Wl,-T"$here/link_boot.ld" -Wl,-Map="$out/boot.map" \
    "$here/boot.c" "$here/crash.c" "$ex/../watchdog/startup_wdt.c" "$root/platform/arm-m4/platform_semihost.c" \
    -o "$out/boot.elf"

# the application: the kernel is built with the configuration of this directory
arm-none-eabi-gcc "${cpu[@]}" "${inc[@]}" "${link[@]}" -Wl,-T"$here/link_app.ld" -Wl,-Map="$out/app.map" \
    "$here/crash_demo.c" "$here/crash.c" "$root/examples/watchdog/wdt.c" "$ex/src/startup_rtos.c" \
    "$root/platform/arm-m4/platform_semihost.c" \
    "$k/tasks.c" "$k/queue.c" "$k/list.c" "$k/timers.c" "$k/portable/GCC/ARM_CM3/port.c" \
    "$k/portable/MemMang/heap_4.c" -w -o "$out/app.elf"

for image in boot app; do
    arm-none-eabi-objcopy -O binary "$out/$image.elf" "$out/$image.bin"
done
(cd "$out" && arm-none-eabi-size boot.elf app.elf && ls -l boot.bin app.bin | awk '{print $5, $9}')
qemu-system-arm -machine mps2-an386 -nographic -icount shift=5,sleep=off \
    -semihosting-config enable=on,target=native \
    -device loader,file="$out/boot.bin",addr=0x00000000 -device loader,file="$out/app.bin",addr=0x00008000 \
    < /dev/null 2>&1
