#!/bin/bash
# Runs a Cortex-M4 image under QEMU, halted at the first instruction, and drives gdb with a command file.
# Usage: tools/debug_qemu.sh <elf> [gdb_command_file]
set -eu

elf=${1:?usage: debug_qemu.sh <elf> [gdb_command_file]}
commands=${2:-tools/debug.gdb}
port=1234

qemu-system-arm -machine mps2-an386 -nographic -semihosting-config enable=on,target=native \
    -S -gdb "tcp::$port" -kernel "$elf" > /dev/null 2>&1 &
qemu=$!
trap 'kill "$qemu" 2> /dev/null || true' EXIT

# Wait until QEMU listens on the port, for at most 5 seconds.
for _ in $(seq 50); do
    (exec 3<> "/dev/tcp/localhost/$port") 2> /dev/null && break
    sleep 0.1
done

gdb-multiarch -q -batch -x "$commands" "$elf"
