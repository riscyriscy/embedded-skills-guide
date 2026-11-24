#!/bin/bash
# Links fmt.c three ways with newlib-nano and prints the size of each image.
# Usage: examples/size/fmt_sizes.sh
set -eu
here=$(cd "$(dirname "$0")" && pwd)
cc="arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -Os -ffunction-sections -fdata-sections -Wl,--gc-sections --specs=nano.specs --specs=nosys.specs"
printf '%-34s %6s %5s %5s\n' variant text data bss
run() {
    label=$1; shift
    $cc "$@" "$here/fmt.c" -o /tmp/fmt.elf > /dev/null 2>&1
    arm-none-eabi-size /tmp/fmt.elf | awk -v l="$label" 'NR == 2 { printf "%-34s %6d %5d %5d\n", l, $1, $2, $3 }'
}
run "hand-written conversion" -DMODE=0
run "snprintf %d" -DMODE=1
run "snprintf %f, nano without float" -DMODE=2
run "snprintf %f, with -u _printf_float" -DMODE=2 -Wl,-u,_printf_float
