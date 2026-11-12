#!/usr/bin/env python3
"""Power cut test. A guest runs writer.c on an ext4 disk, the host kills QEMU at a random moment, replays the
journal of the disk image and reads the record that is left. Killing QEMU is a power cut for everything that the
guest has not yet sent to its disk. Usage:

    powercut.py --image Image --busybox busybox --writer writer cut   [--trials 25]   a table of outcomes
    powercut.py --image Image --busybox busybox --writer writer cost  [--saves 500]   bytes written to the disk
    powercut.py --image Image --busybox busybox --writer writer torn                  the extents of a record that is torn
"""
import argparse
import concurrent.futures as cf
import os
import random
import re
import shutil
import struct
import subprocess
import tempfile
import threading
import time
import zlib

PAGE, PAGES = 4096, 4
SIZE = PAGE * PAGES
METHODS = ["inplace", "truncate", "rename", "durable"]


def classify(data):
    """valid with a generation, or the way in which the record is damaged"""
    if data is None:
        return "missing", None
    if len(data) != SIZE:
        return "short", None
    gens = set()
    for k in range(PAGES):
        m = re.match(rb"PAGE gen=(\d{8}) k=(\d)\n", data[k * PAGE:k * PAGE + 32])
        if not m:
            return "torn", None
        gens.add(int(m.group(1)))
    crc = zlib.crc32(data[:SIZE - 4]) & 0xFFFFFFFF
    if len(gens) != 1 or struct.unpack("<I", data[-4:])[0] != crc:
        return "torn", None
    return "valid", gens.pop()


def make_initramfs(work, busybox, writer):
    root = os.path.join(work, "initramfs")
    os.makedirs(os.path.join(root, "bin"))
    os.makedirs(os.path.join(root, "usr/bin"))
    shutil.copy(busybox, os.path.join(root, "bin/busybox"))
    shutil.copy(writer, os.path.join(root, "usr/bin/writer"))
    shutil.copy(os.path.join(os.path.dirname(os.path.abspath(__file__)), "init"), os.path.join(root, "init"))
    os.chmod(os.path.join(root, "init"), 0o755)
    out = os.path.join(work, "initramfs.cpio.gz")
    subprocess.run(f"cd {root} && find . | cpio -o -H newc --quiet | gzip -9 > {out}", shell=True, check=True)
    return out


def qemu(a, initramfs, disk, method, opts, count=0, fast=False):
    append = f"console=ttyAMA0 rdinit=/init pcmethod={method} pcopts={opts} pccount={count}" + (" pcflush=fast" if fast else "")
    cmd = ["qemu-system-aarch64", "-machine", "virt", "-cpu", "cortex-a57", "-m", "128", "-smp", "1", "-nographic",
           "-no-reboot", "-net", "none", "-kernel", a.image, "-initrd", initramfs, "-append", append,
           "-drive", f"if=none,file={disk},format=raw,id=hd0", "-device", "virtio-blk-device,drive=hd0"]
    return subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)


def trial(a, initramfs, method, opts, seed, inspect=None):
    rng = random.Random(seed)
    with tempfile.TemporaryDirectory() as d:
        disk = os.path.join(d, "data.img")
        shutil.copy(a.pristine, disk)
        p = qemu(a, initramfs, disk, method, opts, fast=True)
        state = {"started": threading.Event(), "acked": 0}

        def reader():
            for raw in p.stdout:
                line = raw.decode(errors="replace").strip()
                if line == "start":
                    state["started"].set()
                m = re.fullmatch(r"saved (\d+)", line)
                if m:
                    state["acked"] = int(m.group(1))
        t = threading.Thread(target=reader, daemon=True)
        t.start()
        if not state["started"].wait(60):
            p.kill()
            raise RuntimeError("the guest did not start")
        time.sleep(rng.uniform(0.3, 3.5))
        p.kill()                                  # the power cut
        p.wait()
        t.join(5)
        subprocess.run(["e2fsck", "-p", "-E", "journal_only", disk], capture_output=True)   # replay, as a mount does
        r = subprocess.run(["debugfs", "-R", "cat /state.bin", disk], capture_output=True)
        data = r.stdout if r.stdout or b"not found" not in r.stderr.lower() else None
        kind, gen = classify(data)
        if inspect:
            inspect(disk, kind, data)
        return kind, gen, state["acked"]


def cut(a, initramfs):
    print(f"{'method':<10}{'mount options':<26}{'cuts':>5}{'valid':>7}{'short':>7}{'torn':>6}{'missing':>9}{'old':>5}")
    seed = 1
    for opts in a.mounts:
        for method in METHODS:
            seeds = list(range(seed, seed + a.trials)); seed += a.trials
            with cf.ThreadPoolExecutor(a.jobs) as pool:
                res = list(pool.map(lambda s: trial(a, initramfs, method, opts, s), seeds))
            n = {k: sum(1 for r in res if r[0] == k) for k in ("valid", "short", "torn", "missing")}
            old = sum(1 for r in res if r[0] == "valid" and r[1] < r[2])    # an acknowledged save was lost
            print(f"{method:<10}{opts:<26}{len(res):>5}{n['valid']:>7}{n['short']:>7}{n['torn']:>6}{n['missing']:>9}{old:>5}", flush=True)


def torn(a, initramfs):
    """repeat the cut of the rename method with the default options of ext4 until the record is torn, then show it"""
    found = []

    def inspect(disk, kind, data):
        if kind == "torn":
            ex = subprocess.run(["debugfs", "-R", "ex /state.bin", disk], capture_output=True, text=True).stdout
            found.append((ex, data))
    for seed in range(1, 200):
        trial(a, initramfs, "rename", "commit=1", seed, inspect)
        if found:
            break
    ex, data = found[0]
    print('$ debugfs -R "ex /state.bin" data.img')
    print(ex.rstrip())
    print(f"size of the file: {len(data)} bytes")
    for k in range(PAGES):
        print(f"first 16 bytes of page {k}: " + " ".join(f"{b:02x}" for b in data[k * PAGE:k * PAGE + 16]))


def cost(a, initramfs):
    for method in METHODS:
        with tempfile.TemporaryDirectory() as d:
            disk = os.path.join(d, "data.img")
            shutil.copy(a.pristine, disk)
            p = qemu(a, initramfs, disk, method, "defaults", count=a.saves)
            out = p.communicate(timeout=300)[0].decode(errors="replace")
        m = re.search(r"bytes to the disk: (\d+)", out)
        written = int(m.group(1))
        print(f"{method:<10}{a.saves} saves of {SIZE} bytes: {written} bytes to the disk, {written / a.saves:.0f} for each save", flush=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--image", required=True, help="kernel Image")
    ap.add_argument("--busybox", required=True)
    ap.add_argument("--writer", required=True)
    ap.add_argument("--trials", type=int, default=25)
    ap.add_argument("--saves", type=int, default=500)
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--mounts", nargs="+", default=["commit=1", "commit=1,noauto_da_alloc"])
    ap.add_argument("command", choices=["cut", "cost", "torn"])
    a = ap.parse_args()
    with tempfile.TemporaryDirectory() as work:
        a.pristine = os.path.join(work, "pristine.img")
        subprocess.run(["truncate", "-s", "16M", a.pristine], check=True)
        subprocess.run(["mke2fs", "-q", "-t", "ext4", a.pristine], check=True)
        initramfs = make_initramfs(work, a.busybox, a.writer)
        {"cut": cut, "cost": cost, "torn": torn}[a.command](a, initramfs)


if __name__ == "__main__":
    main()
