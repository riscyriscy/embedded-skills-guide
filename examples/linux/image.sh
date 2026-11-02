#!/bin/bash
# Assembles the root file system, the initramfs and the disk image from the files of build.sh.
# Usage: examples/linux/image.sh [output directory]
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
out=${1:-$root/build/linux}
mkdir -p "$out"
out=$(cd "$out" && pwd)
fs=$out/rootfs
rm -rf "$fs" && mkdir -p "$fs"

# the directory tree of the root file system
mkdir -p "$fs"/{bin,sbin,usr/bin,usr/sbin,dev,proc,sys,tmp,run,root,etc/init.d,var/log}
cp -a "$out/busybox-install/." "$fs/"
cp -a "$here/rootfs/." "$fs/"
install -m 755 "$out/sensord" "$fs/usr/sbin/sensord"
ln -sf /bin/busybox "$fs/init"

# three forms of the same tree
(cd "$fs" && find . | cpio -o -H newc --quiet | gzip -9 > "$out/rootfs.cpio.gz")
mksquashfs "$fs" "$out/rootfs.squashfs" -comp gzip -noappend -quiet
rm -f "$out/rootfs.ext4"
mke2fs -q -t ext4 -L rootfs -d "$fs" "$out/rootfs.ext4" 16M

# the boot script and the boot partition, which U-Boot reads
mkimage -A arm64 -T script -C none -n sensorfw -d "$here/boot.cmd" "$out/boot.scr" >/dev/null
rm -f "$out/boot.ext4"
mkdir -p "$out/boot" && cp "$out/Image" "$out/boot.scr" "$out/boot/"
mke2fs -q -t ext2 -L boot -d "$out/boot" "$out/boot.ext4" 8M

# the disk: a partition table with the boot partition and the root partition
rm -f "$out/disk.img"
truncate -s 28M "$out/disk.img"
sfdisk -q "$out/disk.img" <<EOT
label: dos
start=2048, size=16384, type=83
start=18432, size=32768, type=83
EOT
dd if="$out/boot.ext4" of="$out/disk.img" bs=512 seek=2048 conv=notrunc status=none
dd if="$out/rootfs.ext4" of="$out/disk.img" bs=512 seek=18432 conv=notrunc status=none
ls -l "$out"/{Image,busybox,sensord,rootfs.cpio.gz,rootfs.squashfs,rootfs.ext4,boot.ext4,disk.img}
