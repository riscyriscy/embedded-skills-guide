#!/usr/bin/env python3
"""Run one Unity test program on a target and report the result.

    target_test.py --elf build/test_frame --junit result.xml          (QEMU, semihosting output)
    target_test.py --elf build/test_frame --uart --junit result.xml   (QEMU, output on the UART)
    target_test.py --device /dev/ttyUSB0 --baud 115200 --junit result.xml   (a board that is running)

The exit status is 0 when every test passed, 1 when a test failed, 2 when the run ended without a
Unity summary, and 3 when the run exceeded its timeout.
"""
import argparse
import os
import re
import select
import subprocess
import sys
import termios
import time
import xml.etree.ElementTree as ET

RESULT = re.compile(r"^(?P<file>[^:]+):(?P<line>\d+):(?P<name>\w+):(?P<state>PASS|FAIL|IGNORE)(?::(?P<message>.*))?$")
SUMMARY = re.compile(r"^(?P<tests>\d+) Tests (?P<failures>\d+) Failures (?P<ignored>\d+) Ignored")
QEMU = ["qemu-system-arm", "-machine", "mps2-an386", "-display", "none", "-monitor", "none",
        "-semihosting-config", "enable=on,target=native"]


def read_lines(stream_fd, deadline, finished):
    """Yield complete lines from a file descriptor until `finished()` is true or time runs out."""
    pending = b""
    while time.monotonic() < deadline:
        ready, _, _ = select.select([stream_fd], [], [], 0.2)
        if ready:
            try:
                chunk = os.read(stream_fd, 4096)
            except OSError:  # the other end of a pseudo terminal has closed
                break
            if not chunk:
                break
            pending += chunk
            while b"\n" in pending:
                line, pending = pending.split(b"\n", 1)
                yield line.decode("utf-8", "replace").rstrip("\r")
        elif finished():
            break
    if pending:
        yield pending.decode("utf-8", "replace").rstrip("\r")


def open_serial(path, baud):
    fd = os.open(path, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)
    attrs[0] = attrs[1] = attrs[3] = 0  # raw input, raw output, no echo
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    speed = getattr(termios, f"B{baud}")
    attrs[4] = attrs[5] = speed
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return fd


def write_junit(path, suite, results, elapsed):
    failures = sum(1 for r in results if r["state"] == "FAIL")
    skipped = sum(1 for r in results if r["state"] == "IGNORE")
    root = ET.Element("testsuite", name=suite, tests=str(len(results)), failures=str(failures),
                      skipped=str(skipped), time=f"{elapsed:.2f}")
    for r in results:
        case = ET.SubElement(root, "testcase", classname=suite, name=r["name"],
                             file=r["file"], line=r["line"])
        if r["state"] == "FAIL":
            ET.SubElement(case, "failure", message=r["message"] or "failed").text = r["message"]
        elif r["state"] == "IGNORE":
            ET.SubElement(case, "skipped", message=r["message"] or "ignored")
    ET.ElementTree(root).write(path, encoding="utf-8", xml_declaration=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--elf", help="program to run in QEMU")
    ap.add_argument("--uart", action="store_true", help="the program writes to the UART, not to semihosting")
    ap.add_argument("--device", help="serial device of a running board")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--timeout", type=float, default=20.0, help="seconds")
    ap.add_argument("--junit", help="write a JUnit XML file")
    ap.add_argument("--quiet", action="store_true", help="do not echo the lines that are read")
    args = ap.parse_args()
    if bool(args.elf) == bool(args.device):
        ap.error("give exactly one of --elf and --device")

    start = time.monotonic()
    deadline = start + args.timeout
    proc = None
    if args.elf:
        cmd = QEMU + ["-kernel", args.elf]
        if args.uart:
            # QEMU opens the slave side of a pseudo terminal as its serial port; this program
            # reads the master side, in the way that it would read a device of a board.
            fd, slave = os.openpty()
            tty = os.ttyname(slave)
            proc = subprocess.Popen(cmd + ["-serial", tty], stdout=subprocess.DEVNULL,
                                    stderr=subprocess.DEVNULL, pass_fds=[])
        else:
            proc = subprocess.Popen(cmd + ["-nographic"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            fd = proc.stdout.fileno()
        finished = lambda: proc.poll() is not None
        suite = os.path.basename(args.elf)
    else:
        fd = open_serial(args.device, args.baud)
        finished = lambda: False
        suite = os.path.basename(args.device)

    results, summary = [], None
    for line in read_lines(fd, deadline, finished):
        if not args.quiet:
            print(line)
        m = RESULT.match(line)
        if m:
            results.append(m.groupdict())
        s = SUMMARY.match(line)
        if s:
            summary = s.groupdict()
            if args.device:
                break  # a board does not exit; the summary is the end of the run
    elapsed = time.monotonic() - start
    timed_out = summary is None and time.monotonic() >= deadline
    if proc is not None:
        if proc.poll() is None:
            proc.kill()
        proc.wait()
    if args.junit:
        write_junit(args.junit, suite, results, elapsed)

    if timed_out:
        print(f"timeout after {args.timeout:.0f} s without a summary line", file=sys.stderr)
        return 3
    if summary is None:
        print("the run ended without a summary line", file=sys.stderr)
        return 2
    return 1 if int(summary["failures"]) else 0


if __name__ == "__main__":
    sys.exit(main())
