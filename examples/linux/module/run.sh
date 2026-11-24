#!/bin/bash
# Packs init and the module into an initramfs and boots the kernel image in QEMU
# (machine virt, arm64). PARAMS="start=5 bad=1" passes module parameters.
# Usage: [MODULE=<file>] examples/linux/module/run.sh <kernel Image>
# The module is built first, see the Makefile.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
image=${1:?usage: run.sh <kernel Image>}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
aarch64-linux-gnu-gcc -static -O2 -Wall -Wextra "$here/init.c" -o "$work/init"
mkdir "$work/root" "$work/root/proc" "$work/root/dev"
cp "$work/init" "$work/root/init"
cp "${MODULE:-$here/sfw_counter.ko}" "$work/root/sfw_counter.ko"
[ -z "${PARAMS:-}" ] || printf %s "$PARAMS" > "$work/root/params"
(cd "$work/root" && find . | cpio -o -H newc --quiet | gzip > "$work/initramfs.gz")
timeout 60 qemu-system-aarch64 -machine virt -cpu cortex-a57 -m 128 -nographic \
    -no-reboot -kernel "$image" -initrd "$work/initramfs.gz" \
    -append "console=ttyAMA0 loglevel=4 rdinit=/init"
