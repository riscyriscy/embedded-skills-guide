#!/bin/bash
# Builds an x86-64 Ubuntu guest with systemd, a kernel and an initramfs, to run the units of examples/linux/systemd
# as they run on a system with systemd. The arm64 image of build.sh uses BusyBox init instead.
# Needs root, debootstrap, mke2fs and the Ubuntu archive. Usage: sudo examples/linux/systemd-guest.sh <directory>
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
dir=${1:?usage: systemd-guest.sh <directory>}
tree=$dir/tree
mkdir -p "$dir"
if [ ! -d "$tree/etc" ]; then
    debootstrap --variant=minbase --include=systemd,systemd-sysv,libpam-systemd,dbus,iproute2,procps,less,kmod \
        noble "$tree" http://archive.ubuntu.com/ubuntu
fi
for d in proc sys dev; do mount --bind /$d "$tree/$d"; done
unmount() { for d in dev sys proc; do mountpoint -q "$tree/$d" && umount "$tree/$d"; done; return 0; }
trap unmount EXIT
chroot "$tree" env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends linux-image-virtual initramfs-tools

# the service of the image, built for the guest, and its units
gcc -O2 -Wall -Wextra -DRINGBUF_CAPACITY=16u -DSENSORD_VERSION="\"$(git -C "$root" describe --always --dirty 2>/dev/null || echo unknown)\"" \
    -I"$root/lib/include" "$here/src/sensord.c" "$root/lib/src/ringbuf.c" "$root/lib/src/stats.c" -o "$tree/usr/sbin/sensord"
cp "$here/rootfs/etc/sensord.period" "$tree/etc/sensord.period"
cp "$here/systemd/sensord.service" "$here/systemd/sensord-naive.service" "$tree/etc/systemd/system/"
mkdir -p "$tree/etc/systemd/system/sensord.service.d"
cp "$here/systemd/sensord.service.d/override.conf" "$tree/etc/systemd/system/sensord.service.d/"
chroot "$tree" systemctl enable sensord.service

# a console that needs no password, and a prompt that the driver of session.py knows
echo sensorfw > "$tree/etc/hostname"
chroot "$tree" passwd -d root
grep -q 'PS1="guest# "' "$tree/root/.bashrc" || printf 'PS1="guest# "\nexport SYSTEMD_URLIFY=0 SYSTEMD_COLORS=0 SYSTEMD_PAGER=\n' >> "$tree/root/.bashrc"
printf '[Journal]\nStorage=volatile\n' > "$tree/etc/systemd/journald.conf"

cp --dereference "$tree/boot/vmlinuz" "$dir/vmlinuz"
cp --dereference "$tree/boot/initrd.img" "$dir/initrd.img"
unmount
rm -f "$dir/guest.ext4"
mke2fs -q -t ext4 -L guest -d "$tree" "$dir/guest.ext4" 900M
ls -l "$dir"/vmlinuz "$dir"/initrd.img "$dir"/guest.ext4
