#!/bin/bash
# Usage: latency.sh Image busybox cyclictest
# Runs cyclictest in QEMU with the instruction counter as the clock (-icount shift=5,sleep=on: the virtual clock
# advances 32 ns for each instruction that runs, and follows the clock of the host while the CPU is idle). A run
# that never idles has a stable average, and its maximum varies by up to a quarter between runs. With sleep=off the arm64 guest stalls in its first sleep. The
# system is an initramfs with BusyBox and cyclictest, and the test is the script init next to this file.
set -eu
kernel=$1; busybox=$2; cyclictest=$3
here=$(cd "$(dirname "$0")" && pwd)
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
mkdir -p "$work/root/bin" "$work/root/usr/bin"
cp "$busybox" "$work/root/bin/busybox"
cp "$cyclictest" "$work/root/usr/bin/cyclictest"
cp "$here/init" "$work/root/init"
(cd "$work/root" && find . | cpio -o -H newc --quiet | gzip -9 > "$work/initramfs.cpio.gz")
timeout -s KILL "${TIMEOUT:-300}" qemu-system-aarch64 -machine virt -cpu cortex-a57 -m 128 -smp 1 -nographic -no-reboot -net none \
    -icount shift=5,sleep=on -kernel "$kernel" -initrd "$work/initramfs.cpio.gz" \
    -append "console=ttyAMA0 rdinit=/init" < /dev/null 2>&1 | tr -d '\r' | sed -n '/^sensorfw# /,/^DONE/p'
