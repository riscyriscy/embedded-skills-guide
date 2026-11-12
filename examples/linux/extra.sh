#!/bin/bash
# Adds the programs of Chapters 20 and 21 to the root file system of image.sh: cyclictest, the tools of libgpiod
# and two small programs that use GPIO and I2C from user space. The sources of rt-tests, numactl and libgpiod
# are not part of the project, and each is built static for arm64 with the cross compiler of build.sh.
# Usage: RT_TESTS_SRC=... NUMA_SRC=... GPIOD_SRC=... examples/linux/extra.sh [output directory of build.sh]
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
out=$(cd "${1:-$root/build/linux}" && pwd)
: "${RT_TESTS_SRC:?set RT_TESTS_SRC to the rt-tests source}"
: "${NUMA_SRC:?set NUMA_SRC to the numactl source}"
: "${GPIOD_SRC:?set GPIOD_SRC to the libgpiod source}"
cross=aarch64-linux-gnu-
jobs=${JOBS:-2}

# libnuma is needed to build cyclictest, and cyclictest is linked static with it
(cd "$NUMA_SRC" && autoreconf -fi && ./configure --host=aarch64-linux-gnu --enable-static --disable-shared \
    && make -j"$jobs" libnuma.la)
make -C "$RT_TESTS_SRC" clean
make -C "$RT_TESTS_SRC" -j"$jobs" CC=${cross}gcc cyclictest CFLAGS="-O2 -Wall -Wno-nonnull -Wno-error -I$NUMA_SRC" \
    LDFLAGS="-static -L$NUMA_SRC/.libs"

# libgpiod with its tools
(cd "$GPIOD_SRC" && ./configure --host=aarch64-linux-gnu --enable-tools=yes --enable-static --disable-shared \
    --prefix="$out/gpiod" CFLAGS="-O2" LDFLAGS="-static" && make -j"$jobs" LDFLAGS="-all-static" && make install LDFLAGS="-all-static")

# the two programs of this project
for p in gpio_demo i2c_demo; do
    ${cross}gcc -O2 -Wall -Wextra -static -I"$out/gpiod/include" "$here/src/$p.c" -L"$out/gpiod/lib" -lgpiod -o "$out/$p"
done

# the root file system of image.sh plus the new programs, as an ext4 image
fs=$out/rootfs-extra
rm -rf "$fs" && cp -a "$out/rootfs" "$fs"
install -m 755 "$RT_TESTS_SRC/cyclictest" "$out/gpiod/bin/"* "$out/gpio_demo" "$out/i2c_demo" "$fs/usr/bin/"
# the module that simulates an I2C bus: build it with "make drivers/i2c/i2c-stub.ko" in a kernel tree that uses
# hw/kernel-hw.fragment, and copy it to $out/i2c-stub.ko
[ -f "$out/i2c-stub.ko" ] && install -D -m 644 "$out/i2c-stub.ko" "$fs/lib/modules/i2c-stub.ko"
rm -f "$out/rootfs-extra.ext4"
mke2fs -q -t ext4 -L rootfs -d "$fs" "$out/rootfs-extra.ext4" 32M
ls -l "$out"/gpio_demo "$out"/i2c_demo "$RT_TESTS_SRC/cyclictest" "$out/rootfs-extra.ext4"
