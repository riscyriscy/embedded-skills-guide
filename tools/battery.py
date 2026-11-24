#!/usr/bin/env python3
"""Average current and battery life of a duty cycled device.

Usage: battery.py <active mA> <active ms> <period s> <sleep uA> <capacity mAh>
The device is active for <active ms> at <active mA> once every <period s> and sleeps
for the rest of the period. All five numbers are inputs: take them from
measurements, not from data sheet typical values, and use the capacity that the
battery gives at your current and temperature.
"""
import sys


def main():
    if len(sys.argv) != 6:
        sys.exit(__doc__)
    active_ma, active_ms, period_s, sleep_ua, capacity_mah = map(float, sys.argv[1:])
    active_s = active_ms / 1000.0
    if active_s >= period_s:
        sys.exit("the active time must be shorter than the period")
    charge_uc = active_ma * 1000.0 * active_s + sleep_ua * (period_s - active_s)
    average_ua = charge_uc / period_s
    hours = capacity_mah * 1000.0 / average_ua
    print(f"duty cycle:      {100.0 * active_s / period_s:.3f} percent")
    print(f"average current: {average_ua:.2f} uA")
    print(f"active share:    {100.0 * active_ma * 1000.0 * active_s / charge_uc:.1f}"
          " percent of the charge")
    print(f"battery life:    {hours:.0f} hours, {hours / 24.0 / 365.0:.1f} years")


if __name__ == "__main__":
    main()
