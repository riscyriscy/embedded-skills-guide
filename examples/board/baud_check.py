#!/usr/bin/env python3
"""Baud rate of a UART from the clock that drives it and the divisor that the firmware writes.

Usage: baud_check.py CLOCK_HZ BAUD [ASSUMED_CLOCK_HZ]
The divisor is computed for ASSUMED_CLOCK_HZ (default CLOCK_HZ), the clock that the firmware believes it has.
The UART then runs from CLOCK_HZ, the clock that the part really has.
"""
import sys


def main(argv):
    if len(argv) not in (3, 4):
        sys.exit(__doc__)
    real, baud = float(argv[1]), float(argv[2])
    assumed = float(argv[3]) if len(argv) == 4 else real
    divisor = round(assumed / baud)
    actual = real / divisor
    error = (actual - baud) / baud * 100
    print(f"divisor {divisor}, actual baud rate {actual:.0f}, error {error:+.2f} percent")
    print(f"one bit {1e6 / actual:.2f} us, one byte of 10 bits {10e6 / actual:.1f} us, 80 bytes {80 * 10e3 / actual:.2f} ms")
    limit = 0.5 / 9.5 * 100
    verdict = "inside" if abs(error) < limit else "outside"
    print(f"the stop bit is sampled 9.5 bit times after the start edge, so the errors of both ends together must stay below {limit:.1f} percent: this end alone is {verdict}")


if __name__ == "__main__":
    main(sys.argv)
