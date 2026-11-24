# examples/security

Programs for Chapter 15 of Volume 2 of the Embedded Skills Guide, security practice.

| File | Purpose |
|---|---|
| `stack_guard.c` | an overflow of a stack buffer, with and without `-fstack-protector-strong` |
| `startup_mpu.c`, `mpu_demo.c` | MPU regions on a Cortex-M4: a write to read only memory (`-DTEST=1`) and a fetch from RAM (`-DTEST=2`) |
| `compare.c`, `count.sh` | a leaky and a constant time comparison, and the instruction counts of both (needs valgrind) |
| `secure_entry.c`, `secure.ld` | a TrustZone-M secure entry point, compiled and linked with `-mcmse` and not run |

Run the guard:

```
STARTUP=examples/practice/startup_irq.c examples/run.sh m4 -O1 -Wall -Wextra \
    -fstack-protector-strong examples/security/stack_guard.c
```
