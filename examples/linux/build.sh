#!/bin/bash
# Builds the pieces of the image for the QEMU machine "virt" (arm64) with the cross compiler of Ubuntu:
# the kernel, BusyBox and the service. The sources are not part of the project.
# Usage: LINUX_SRC=... BUSYBOX_SRC=... examples/linux/build.sh [output directory]
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
out=${1:-$root/build/linux}
: "${LINUX_SRC:?set LINUX_SRC to a kernel source tree}"
: "${BUSYBOX_SRC:?set BUSYBOX_SRC to a BusyBox source tree}"
cross=aarch64-linux-gnu-
jobs=${JOBS:-2}
mkdir -p "$out"
out=$(cd "$out" && pwd)

# the kernel: tinyconfig, then the options of kernel.fragment
make -C "$LINUX_SRC" ARCH=arm64 CROSS_COMPILE=$cross tinyconfig
"$LINUX_SRC/scripts/kconfig/merge_config.sh" -m -O "$LINUX_SRC" "$LINUX_SRC/.config" "$here/kernel.fragment"
make -C "$LINUX_SRC" ARCH=arm64 CROSS_COMPILE=$cross olddefconfig
make -C "$LINUX_SRC" ARCH=arm64 CROSS_COMPILE=$cross -j"$jobs" Image
cp "$LINUX_SRC/arch/arm64/boot/Image" "$LINUX_SRC/.config" "$out/"
mv "$out/.config" "$out/kernel.config"

# BusyBox: the default configuration, then the options of busybox.fragment
make -C "$BUSYBOX_SRC" CROSS_COMPILE=$cross defconfig
# BusyBox has no merge script: delete each option of the fragment from .config, then append it
grep -E '^(CONFIG_|# CONFIG_)' "$here/busybox.fragment" | while read -r line; do
    name=$(echo "$line" | sed -E 's/^# //; s/[ =].*//')
    sed -i "/^$name=/d; /^# $name is not set/d" "$BUSYBOX_SRC/.config"
    echo "$line" >> "$BUSYBOX_SRC/.config"
done
make -C "$BUSYBOX_SRC" CROSS_COMPILE=$cross oldconfig </dev/null >/dev/null
make -C "$BUSYBOX_SRC" CROSS_COMPILE=$cross -j"$jobs" busybox
cp "$BUSYBOX_SRC/busybox" "$out/busybox"
rm -rf "$out/busybox-install"
make -C "$BUSYBOX_SRC" CROSS_COMPILE=$cross CONFIG_PREFIX="$out/busybox-install" install >/dev/null

# the service, linked with the sensor library of the project
version=$(git -C "$root" describe --always --dirty 2>/dev/null || echo unknown)
${cross}gcc -static -O2 -Wall -Wextra -DRINGBUF_CAPACITY=16u -DSENSORD_VERSION="\"$version\"" \
    -I"$root/lib/include" "$here/src/sensord.c" "$root/lib/src/ringbuf.c" "$root/lib/src/stats.c" -o "$out/sensord"
ls -l "$out"
