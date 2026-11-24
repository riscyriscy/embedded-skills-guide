# examples/watchdog

Programs for Chapter 13 of Volume 2 of the Embedded Skills Guide, watchdogs and reset reasons. They run under QEMU (machine mps2-an386) with the start-up code and the linker script in this folder.

| File | Purpose |
|---|---|
| `wdt.c`, `wdt.h` | driver of the CMSDK watchdog, with progress bits for the tasks |
| `reset_reason.c`, `reset_reason.h` | the reason for the last reset, in a record in the .noinit section |
| `link_noinit.ld` | the project linker script plus the .noinit section |
| `startup_wdt.c` | the project start-up code plus HardFault and SysTick vectors |
| `wdt_demo.c` | four resets in one run: a stuck task, a hard fault, a software request, the end |
| `wdt_isr_demo.c` | the mistake of feeding the watchdog from a timer interrupt |
| `wdt_safe_demo.c` | a reset loop guard: after three unplanned resets in a row the program enters a safe mode |

Run:

```
LD_SCRIPT=examples/watchdog/link_noinit.ld STARTUP=examples/watchdog/startup_wdt.c \
    examples/run.sh m4 -O1 -Wall -Wextra -Iexamples/watchdog \
    examples/watchdog/wdt_demo.c examples/watchdog/reset_reason.c examples/watchdog/wdt.c
```
