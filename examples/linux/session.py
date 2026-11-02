#!/usr/bin/env python3
"""Runs the Linux image of examples/linux in QEMU and drives its console, so that a boot or a shell session
can be repeated and captured. Examples:

    session.py --out build/linux --until "Run /sbin/init"            direct kernel boot, stop at a line
    session.py --out build/linux --uboot -- "ps" "cat /proc/version"  boot through U-Boot, run commands
    session.py --out build/linux --append "init=/bin/sh" -- "ls /run"

The commands after "--" run in the shell of the image, one at a time. The transcript goes to standard output
without the carriage returns and the escape sequences of the terminal.
"""
import argparse
import os
import re
import sys
import time

import pexpect

PROMPT = r"sensorfw# "
ANSI = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]|\x1b[78]")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="build/linux", help="directory with the files of build.sh and image.sh")
    ap.add_argument("--guest", help="directory of systemd-guest.sh: boot the x86-64 guest with systemd and log in as root")
    ap.add_argument("--uboot", action="store_true", help="boot through U-Boot from disk.img instead of -kernel")
    ap.add_argument("--disk", help="root disk (default: rootfs.ext4 for -kernel, disk.img for U-Boot)")
    ap.add_argument("--disk2", help="a second virtio disk")
    ap.add_argument("--initrd", help="an initramfs for -kernel")
    ap.add_argument("--append", default="", help="kernel command line, added to the defaults")
    ap.add_argument("--no-defaults", action="store_true", help="do not add console=, root= and rootwait")
    ap.add_argument("--until", help="stop when this regular expression appears in the output")
    ap.add_argument("--keys", action="append", default=[], help="send this text to U-Boot at the autoboot prompt (can repeat)")
    ap.add_argument("--uboot-cmd", action="append", default=[], help="a command for the U-Boot console (after --keys); the last one may start the kernel")
    ap.add_argument("--mark", action="append", default=[], help="print the elapsed time at which this regular expression appears (can repeat)")
    ap.add_argument("--timeout", type=int, default=60)
    ap.add_argument("--prompt", default=PROMPT)
    ap.add_argument("--from-line", help="print from the first line that matches this regular expression")
    ap.add_argument("--final", default="poweroff", help="the last command, which ends the session")
    ap.add_argument("--qemu-extra", default="", help="more options for QEMU, separated by spaces")
    ap.add_argument("commands", nargs="*")
    a = ap.parse_args()

    out = a.out
    if a.guest:
        a.prompt = "guest# " if a.prompt == PROMPT else a.prompt
        cmd = ["qemu-system-x86_64", "-machine", "q35", "-cpu", "qemu64", "-m", "512", "-smp", "2", "-nographic",
               "-no-reboot", "-net", "none", "-kernel", os.path.join(a.guest, "vmlinuz"),
               "-initrd", os.path.join(a.guest, "initrd.img"),
               "-append", "console=ttyS0 root=/dev/vda rw " + a.append,
               "-drive", f"if=none,file={os.path.join(a.guest, 'guest.ext4')},format=raw,id=hd0",
               "-device", "virtio-blk-pci,drive=hd0"] + a.qemu_extra.split()
    base = ["qemu-system-aarch64", "-machine", "virt", "-cpu", "cortex-a57", "-m", "128", "-smp", "1",
            "-nographic", "-no-reboot", "-net", "none"]
    if a.guest:
        pass
    elif a.uboot:
        disk = a.disk or os.path.join(out, "disk.img")
        cmd = base + ["-bios", "/usr/lib/u-boot/qemu_arm64/u-boot.bin"]
    else:
        disk = a.disk or os.path.join(out, "rootfs.ext4")
        append = "" if a.no_defaults else "console=ttyAMA0 root=/dev/vda rootwait ro "
        cmd = base + ["-kernel", os.path.join(out, "Image"), "-append", append + a.append]
        if a.initrd:
            cmd += ["-initrd", a.initrd]
    if not a.guest and disk != "none":
        cmd += ["-drive", f"if=none,file={disk},format=raw,id=hd0", "-device", "virtio-blk-device,drive=hd0"]
    if a.disk2 and not a.guest:
        cmd += ["-drive", f"if=none,file={a.disk2},format=raw,id=hd1,readonly=on", "-device", "virtio-blk-device,drive=hd1"]
    if not a.guest:
        cmd += a.qemu_extra.split()

    cmd.insert(1, "-snapshot")      # writes go to a temporary file, so every session starts from the same disk
    child = pexpect.spawn(cmd[0], cmd[1:], encoding="utf-8", timeout=a.timeout, codec_errors="replace")
    log = []
    marks = []
    child.logfile_read = type("L", (), {"write": lambda s, t: log.append(t), "flush": lambda s: None})()
    try:
        if a.uboot and a.keys:
            child.expect(r"Hit any key to stop autoboot")
            for k in a.keys:
                child.send(k)
                child.expect(r"=> ")
            for c in a.uboot_cmd:
                child.sendline(c)
                if c.startswith("booti") or c.startswith("boot"):
                    break
                child.expect(r"=> ")
        if a.mark:
            start = time.monotonic()
            for m in a.mark:
                child.expect(m)
                marks.append((time.monotonic() - start, m))
        elif a.until:
            child.expect(a.until)
            child.expect(r"[\r\n]", timeout=3)    # let the rest of the matching line arrive
        elif a.commands:
            if a.guest:
                child.expect("login: ")
                child.sendline("root")
            child.expect(a.prompt)
            for c in a.commands:
                child.sendline(c)
                child.expect(a.prompt)
            child.sendline(a.final)
            child.expect(pexpect.EOF)
        else:
            child.expect(pexpect.EOF)
    except (pexpect.TIMEOUT, pexpect.EOF):
        pass
    finally:
        if child.isalive():
            child.terminate(force=True)
    text = ANSI.sub("", "".join(log)).replace("\r", "")
    while re.search(r"[^\n]\x08", text):
        text = re.sub(r"[^\n]\x08", "", text)
    text = text.replace("\x08", "")
    lines = text.split("\n")
    if a.from_line:
        for i, line in enumerate(lines):
            if re.search(a.from_line, line):
                lines = lines[i:]
                break
    if marks:
        for seconds, m in marks:
            print(f"{seconds:6.1f} s  {m}")
        return
    sys.stdout.write("\n".join(lines).rstrip("\n") + "\n")


if __name__ == "__main__":
    main()
