# examples/watchdog

Programs that an earlier edition of the book described and that no chapter uses now. They run under QEMU (machine mps2-an386) with the start-up code and the linker script in this folder.

| File | Purpose |
|---|---|
| `wdt.c`, `wdt.h` | driver of the CMSDK watchdog, with progress bits for the tasks |
| `reset_reason.c`, `reset_reason.h` | the reason for the last reset, in a record in the .noinit section |
| `link_noinit.ld` | the project linker script plus the .noinit section |
| `startup_wdt.c` | the project start-up code plus HardFault and SysTick vectors |
| `wdt_demo.c` | four resets in one run: a stuck task, a hard fault, a software request, the end |
| `wdt_isr_demo.c` | the mistake of feeding the watchdog from a timer interrupt |

Run:

```
LD_SCRIPT=examples/watchdog/link_noinit.ld STARTUP=examples/watchdog/startup_wdt.c \
    examples/run.sh m4 -O1 -Wall -Wextra -Iexamples/watchdog \
    examples/watchdog/wdt_demo.c examples/watchdog/reset_reason.c examples/watchdog/wdt.c
```
