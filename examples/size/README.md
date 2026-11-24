# examples/size

Programs for Chapter 18 of Volume 1 of the Embedded Skills Guide, size and speed.

| File | Purpose |
|---|---|
| `variants.sh` | builds one FreeRTOS demo with seven option sets, runs each in QEMU and prints the sizes |
| `fmt.c`, `fmt_sizes.sh` | one number formatted three ways, and the size of each with newlib-nano |
| `hot.c` | a loop that spends its instructions in two functions, for callgrind; `-DFAST` removes the cost |

Run: `examples/size/variants.sh` (needs the Arm toolchain, CMake, Ninja and QEMU). `hot.c` needs valgrind.
