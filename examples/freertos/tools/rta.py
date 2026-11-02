#!/usr/bin/env python3
"""Response time analysis for fixed priority tasks, and the utilization bound of Liu and Layland.

Usage: rta.py name:period_ms:work_ms ...     (the first task has the highest priority)

The worst response time of a task is the least fixed point of
    R = C + sum over the tasks of higher priority of ceil(R / T) * C
It holds for tasks released together, with a deadline equal to the period, no blocking and no
overhead for the kernel."""
import math
import sys


def main(argv):
    tasks = []
    for arg in argv[1:]:
        name, period, work = arg.split(":")
        tasks.append((name, float(period), float(work)))
    if not tasks:
        print(__doc__)
        return 2
    total = sum(c / t for _, t, c in tasks)
    n = len(tasks)
    bound = n * (2 ** (1.0 / n) - 1)
    print(f"utilization {total:.3f}, bound for {n} tasks {bound:.3f}")
    ok = True
    for i, (name, period, work) in enumerate(tasks):
        r = work
        while True:
            nxt = work + sum(math.ceil(r / t) * c for _, t, c in tasks[:i])
            if nxt == r or nxt > period:
                r = nxt
                break
            r = nxt
        met = r <= period
        ok = ok and met
        print(f"{name}: period {period:g} ms, work {work:g} ms, response {r:g} ms, {'meets' if met else 'misses'} the deadline")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
