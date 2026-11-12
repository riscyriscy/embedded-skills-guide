#!/bin/bash
# Builds a layered root from the files of build.sh and image.sh: two versions of the read-only image, an
# 8 MiB data partition for the writable layer, and an initramfs whose init script joins them with the overlay
# file system. The kernel needs CONFIG_OVERLAY_FS, which overlay/kernel-overlay.fragment adds.
# Usage: examples/linux/overlay.sh [output directory of build.sh]
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
out=$(cd "${1:-$root/build/linux}" && pwd)
ov=$out/overlay
rm -rf "$ov" && mkdir -p "$ov/v1" "$ov/v2" "$ov/initramfs/bin"

# version 1 is the tree of image.sh with two empty directories for the mount points of the layers
cp -a "$out/rootfs/." "$ov/v1/"
mkdir -p "$ov/v1/ro" "$ov/v1/rw"
mksquashfs "$ov/v1" "$ov/root-v1.squashfs" -comp gzip -noappend -quiet -no-progress

# version 2 changes two files, as a product update would
cp -a "$ov/v1/." "$ov/v2/"
echo sensorfw-2 > "$ov/v2/etc/hostname"
echo 5 > "$ov/v2/etc/sensord.period"
mksquashfs "$ov/v2" "$ov/root-v2.squashfs" -comp gzip -noappend -quiet -no-progress

# the writable layer lives on its own partition, and the initramfs holds BusyBox and the init script
truncate -s 8M "$ov/data.img"
mke2fs -q -t ext4 -L data "$ov/data.img"
cp "$out/busybox" "$ov/initramfs/bin/busybox"
install -m 755 "$here/overlay/init" "$ov/initramfs/init"
(cd "$ov/initramfs" && find . | cpio -o -H newc --quiet | gzip -9 > "$ov/initramfs.cpio.gz")
ls -l "$ov"/root-v1.squashfs "$ov"/root-v2.squashfs "$ov"/data.img "$ov"/initramfs.cpio.gz
