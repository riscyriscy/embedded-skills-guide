#!/usr/bin/env python3
"""Turns the raw trace of trace_demo ("T <time> <id>" lines) into a timeline and a
summary.

Usage: decode.py <file> --period <counts between two ticks>
(a file of "-" reads standard input)
The time is in counts of the trace timer. The period is the reload value of the tick
timer, in counts of the trace timer.
"""
import argparse
import sys

NAMES = {1: "isr enter", 2: "isr exit", 3: "sample begin", 4: "sample end",
         5: "log begin", 6: "log end"}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("file")
    ap.add_argument("--period", type=int, required=True)
    ap.add_argument("--lines", type=int, default=14,
                    help="records to print in the timeline")
    args = ap.parse_args()
    src = sys.stdin if args.file == "-" else open(args.file)
    records = []
    for line in src:
        parts = line.split()
        if len(parts) == 3 and parts[0] == "T":
            records.append((int(parts[1]), int(parts[2])))

    print(f"{'time':>8}  {'since last':>10}  event")
    last = records[0][0]
    for t, i in records[: args.lines]:
        print(f"{t:8d}  {t - last:10d}  {NAMES.get(i, i)}")
        last = t

    enters = [t for t, i in records if i == 1]
    # The ticks come on a grid of one period. The least late tick fixes the grid.
    offset = min(t % args.period for t in enters)
    late = [(t, (t - offset) % args.period) for t in enters]
    lost, previous = 0, 0
    for t in enters:
        index = (t - offset) // args.period
        lost += index - previous - 1  # ticks inside one interrupts-off section
        previous = index
    spans = []
    begin = None
    for t, i in records:
        if i == 5:
            begin = t
        elif i == 6 and begin is not None:
            spans.append(t - begin)
            begin = None
    worst = max(late, key=lambda x: x[1])
    print()
    print(f"records: {len(records)}, from {records[0][0]} to {records[-1][0]}")
    started_late = sum(1 for _, d in late if d > 0)
    print(f"tick interrupts: {len(enters)}, started late: {started_late}, "
          f"ticks lost: {lost}")
    print(f"worst start latency: {worst[1]} counts, "
          f"for the tick of time {worst[0] - worst[1]}")
    print(f"interrupts-off sections (log begin to log end): {len(spans)}, "
          f"longest {max(spans)} counts")


if __name__ == "__main__":
    main()
