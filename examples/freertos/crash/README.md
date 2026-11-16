# examples/freertos/crash

Program for The Embedded FreeRTOS Handbook. A bootloader and an application share a crash record: a block of RAM at a fixed address that start-up does not clear. The bootloader reads the record and starts the application. The application reports the last reset and then provokes the next one, so that one run shows six starts. Both images run under QEMU (machine mps2-an386, a Cortex-M4) with the FreeRTOS V11.3.3 of this project.

| File | Purpose |
|---|---|
| `persist.ld` | address and size of the region that holds the record, included by both linker scripts |
| `link_boot.ld` | linker script of the bootloader: 32 KB of flash at address 0 |
| `link_app.ld` | linker script of the application: 480 KB of flash at 0x8000 |
| `crash.c`, `crash.h` | the record, the four classes of reset, and the crash path with the stack dump of every task (compiled into both images) |
| `freertos_tasks_c_additions.h` | compiled into the kernel's `tasks.c`: shows every task to the crash path, so that the stack of each task is dumped |
| `boot.c` | the bootloader: reports the last reset and starts the application |
| `crash_demo.c` | the application: reports the last reset, then provokes the next one |
| `FreeRTOSConfig.h` | the configuration of the other FreeRTOS examples, with `configASSERT` calling `crash_system` and the task walk switched on |
| `build.sh` | builds both images, converts them with objcopy and runs them together |

The application uses the watchdog driver of `examples/watchdog` for its fifth start.

Run:

```
examples/freertos/crash/build.sh
```

Build products go to `out/`. Set `OUT` to use another directory.
