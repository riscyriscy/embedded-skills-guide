# examples/practice

Programs for Chapter 17 of Volume 1 of the Embedded Skills Guide, firmware construction practice. All run under QEMU (machine mps2-an386) or on the host.

| File | Purpose |
|---|---|
| `startup_irq.c` | project start-up code plus SysTick and HardFault vectors |
| `isr_flag.c` | a flag shared with an interrupt: hangs at -O1 and -O2 unless built with `-DTICK_VOLATILE` |
| `mmio_timer.c`, `registers.c` | registers as volatile structures, and what the compiler does with a plain access |
| `controller.c`, `controller.h`, `test_controller.c` | a state machine as a table, with a host test |
| `require.h`, `require_debug.c`, `require_reset.c`, `require_demo.c`, `test_require.c` | a REQUIRE macro for programming mistakes, with a debug and a reset handler |
| `portable.c` | atomics and division on Cortex-M0+, M4 and M33, compiled and read, not run |

Run an interrupt example:

```
STARTUP=examples/practice/startup_irq.c timeout 5 examples/run.sh m4 -O2 -DTICK_VOLATILE \
    -Wall -Wextra examples/practice/isr_flag.c
```
