# The boot script of U-Boot. It loads the kernel from the first partition and starts it with the
# device tree that U-Boot received from the machine (here: QEMU).
setenv bootargs "console=ttyAMA0 root=/dev/vda2 rootwait ro"
load virtio 0:1 ${kernel_addr_r} Image
booti ${kernel_addr_r} - ${fdtcontroladdr}
