# examples/linux/module

A kernel module for Chapter 16 of Volume 2 of the Embedded Skills Guide.

| File | Purpose |
|---|---|
| `sfw_counter.c`, `Makefile` | a character device that counts reads; the parameter `bad=1` makes it dereference a null pointer |
| `init.c` | the first program of the test image: loads the module, reads, removes it, prints the kernel messages |
| `run.sh` | packs `init` and the module into an initramfs and boots a kernel image in QEMU (machine virt, arm64) |
| `kernel.fragment` | the options added to `make tinyconfig` for the kernel that the module needs |

Build against a configured and built kernel tree, then run:

```
make -C examples/linux/module KDIR=<kernel tree> ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
examples/linux/module/run.sh <kernel tree>/arch/arm64/boot/Image
PARAMS="start=100 bad=1" examples/linux/module/run.sh <kernel tree>/arch/arm64/boot/Image
```

Build products (`.ko`, `.o`, `Module.symvers`) are not part of the repository.
