# examples/trace

Programs for Chapter 14 of Volume 2 of the Embedded Skills Guide, tracing in RAM.

| File | Purpose |
|---|---|
| `trace.c`, `trace.h` | a ring of event records with timestamps, lock free, safe in interrupts |
| `trace_demo.c` | a 1 ms tick and a main loop that disables interrupts for a slow log write |
| `run.sh` | builds the demo and runs it in QEMU with a virtual clock, so every run gives the same trace |
| `decode.py` | turns the raw trace into a timeline and a summary of late and lost ticks |

Run:

```
examples/trace/run.sh > /tmp/trace.txt
python3 examples/trace/decode.py /tmp/trace.txt --period 25000
```
