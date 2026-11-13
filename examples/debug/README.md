# examples/debug

Programs and gdb command files for Chapter 27.

| File | Purpose |
|---|---|
| `lookup.c` | a lookup that returns NULL; crash.gdb and cond.gdb find it |
| `overrun.c` | a string copy that overwrites its neighbour; watch.gdb finds it |
| `fault_demo.c` | faults of a Cortex-M4 (bus error, division by zero, bad instruction set state) |
| `fault_startup.c` | start-up code with a fault handler that saves the stacked registers |
| `cmfault.py` | gdb command that decodes the fault status registers from the saved copy |
| `fault.gdb` | the gdb session for tools/debug_qemu.sh |

Build the Cortex-M4 image and start the session:

```
PLAT_OPTS="-g -O1" OUT=/tmp/fault.elf STARTUP=examples/debug/src/fault_startup.c \
    examples/run.sh m4 -g -O0 -DFAULT_KIND=1 -DENABLE_FAULT_HANDLERS examples/debug/src/fault_demo.c
tools/debug_qemu.sh /tmp/fault.elf examples/debug/fault.gdb
```
