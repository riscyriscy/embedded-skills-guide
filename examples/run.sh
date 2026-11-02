#!/bin/bash
# Builds example sources for the host or for the Cortex-M4 and runs the result.
# Usage: examples/run.sh <host|m4> [compiler options] <sources>
# Sources ending in .cpp are compiled with the C++ compiler. Set OUT=<file> to keep the image, and
# STARTUP=<file> and LD_SCRIPT=<file> to replace the start-up code and the linker script of the target.
set -eu

target=${1:?usage: run.sh <host|m4> [compiler options] <sources>}
shift
root=$(git rev-parse --show-toplevel)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
out=${OUT:-$work/image.elf}

options=() sources=()
for arg in "$@"; do
    case $arg in
        -*) options+=("$arg") ;;
        *) sources+=("$arg") ;;
    esac
done

if [ "$target" = host ]; then
    cc=gcc cxx=g++
    cpu=() link=() plat=("$root/platform/host/platform_host.c")
else
    cc=arm-none-eabi-gcc cxx=arm-none-eabi-g++
    cpu=(-mcpu=cortex-m4 -mthumb -mfloat-abi=soft -ffunction-sections -fdata-sections)
    link=(--specs=nano.specs -nostartfiles "-Wl,--gc-sections" "-T${LD_SCRIPT:-$root/platform/arm-m4/link.ld}")
    plat=("${STARTUP:-$root/platform/arm-m4/startup.c}" "$root/platform/arm-m4/platform_semihost.c")
fi

objects=() driver=$cc
for src in "${plat[@]}"; do
    "$cc" "${cpu[@]}" -O1 -fno-tree-loop-distribute-patterns -I"$root/platform/include" -c "$src" -o "$work/$(basename "$src").o"
    objects+=("$work/$(basename "$src").o")
done
for src in "${sources[@]}"; do
    compiler=$cc
    case $src in *.cpp) compiler=$cxx driver=$cxx ;; esac
    "$compiler" "${cpu[@]}" "${options[@]}" -I"$root/platform/include" -I"$root/examples/common" -c "$src" -o "$work/$(basename "$src").o"
    objects+=("$work/$(basename "$src").o")
done
"$driver" "${cpu[@]}" "${options[@]}" "${objects[@]}" "${link[@]}" -o "$out"

if [ "$target" = host ]; then
    "$out"
else
    qemu-system-arm -machine mps2-an386 -nographic -semihosting-config enable=on,target=native \
        -kernel "$out" < /dev/null 2>&1
fi
