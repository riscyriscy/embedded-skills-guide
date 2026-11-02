#!/bin/bash
# Starts the image in QEMU: U-Boot is the firmware, and it boots the kernel from the disk.
# Usage: examples/linux/run.sh [disk image] [extra qemu options]
here=$(cd "$(dirname "$0")" && pwd)
disk=${1:-$here/../../build/linux/disk.img}
shift || true
uboot=${UBOOT:-/usr/lib/u-boot/qemu_arm64/u-boot.bin}
exec qemu-system-aarch64 -machine virt -cpu cortex-a57 -m 128 -smp 1 -nographic -no-reboot \
    -bios "$uboot" -drive if=none,file="$disk",format=raw,id=hd0 -device virtio-blk-device,drive=hd0 "$@"
