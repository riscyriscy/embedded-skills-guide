#!/usr/bin/env python3
"""Simulates the A/B update of a device: two slots, a boot counter and a rollback.

A slot file is the payload followed by the SHA-256 of the payload. The hash is written last, so a slot
whose write was cut by a power loss does not verify. The state file (active slot, pending slot, tries left)
is replaced by writing a new file and renaming it, which is atomic on a POSIX file system.

Usage: ab_sim.py --dir DIR init IMAGE VERSION
       ab_sim.py --dir DIR install IMAGE VERSION [--cut-after BYTES] [--cut-state]
       ab_sim.py --dir DIR boot [--bad-app]
       ab_sim.py --dir DIR confirm
       ab_sim.py --dir DIR status
       ab_sim.py selftest IMAGE
"""
import argparse
import hashlib
import json
import os
import shutil
import sys
import tempfile
from pathlib import Path

TRIES = 3
CHUNK = 64


class PowerCut(Exception):
    """Raised where the power fails in the simulation."""


def other(slot):
    return "b" if slot == "a" else "a"


class Device:
    def __init__(self, directory):
        self.dir = Path(directory)
        self.state_file = self.dir / "state.json"

    # ---- storage
    def load(self):
        return json.loads(self.state_file.read_text())

    def save(self, state, cut=False):
        if cut:
            # The new state was written to a temporary file and the power failed before the rename.
            (self.dir / "state.json.tmp").write_text(json.dumps(state))
            raise PowerCut("power lost before the state file was replaced")
        tmp = self.dir / "state.json.tmp"
        tmp.write_text(json.dumps(state))
        os.replace(tmp, self.state_file)

    def write_slot(self, slot, payload, cut_after=None):
        data = payload + hashlib.sha256(payload).digest()  # the hash is the last thing written
        path = self.dir / f"slot-{slot}.img"
        with open(path, "wb") as f:
            for off in range(0, len(data), CHUNK):
                end = off + CHUNK
                if cut_after is not None and end > cut_after:
                    f.write(data[off:cut_after])
                    raise PowerCut(f"power lost after {cut_after} of {len(data)} bytes of slot {slot}")
                f.write(data[off:end])
                f.flush()

    def slot_valid(self, slot):
        path = self.dir / f"slot-{slot}.img"
        if not path.exists():
            return False
        raw = path.read_bytes()
        return len(raw) > 32 and hashlib.sha256(raw[:-32]).digest() == raw[-32:]

    # ---- operations
    def init(self, image, version):
        self.dir.mkdir(parents=True, exist_ok=True)
        self.write_slot("a", Path(image).read_bytes())
        self.save({"active": "a", "pending": None, "tries": 0, "versions": {"a": version, "b": None}})

    def install(self, image, version, cut_after=None, cut_state=False):
        state = self.load()
        target = other(state["active"])
        # The slot that is going to be overwritten is not bootable from this moment.
        state["versions"][target] = None
        self.save(state)
        self.write_slot(target, Path(image).read_bytes(), cut_after)
        state["versions"][target] = version
        state["pending"], state["tries"] = target, TRIES
        self.save(state, cut=cut_state)

    def boot(self):
        """The decision of the boot loader. Returns (slot, version, reason)."""
        state = self.load()
        pending = state["pending"]
        if pending:
            if not self.slot_valid(pending):
                reason = f"slot {pending} failed verification, rolled back"
                state["pending"] = None
            elif state["tries"] == 0:
                reason = f"slot {pending} was not confirmed in {TRIES} boots, rolled back"
                state["pending"] = None
            else:
                state["tries"] -= 1
                self.save(state)
                return pending, state["versions"][pending], f"trial boot, {state['tries']} tries left"
            self.save(state)
            active = state["active"]
            return active, state["versions"][active], reason
        active = state["active"]
        return active, state["versions"][active], "normal boot"

    def confirm(self):
        state = self.load()
        if state["pending"]:
            state["active"], state["pending"], state["tries"] = state["pending"], None, 0
            self.save(state)
        return state["active"]


def selftest(image):
    """Cuts the power at every step of an update and checks that the device still boots a valid image."""
    size = len(Path(image).read_bytes())
    cuts = [("slot", n) for n in range(0, size + 33, 16)] + [("state", None), ("none", None)]
    results = {"old": 0, "new": 0}
    for kind, n in cuts:
        with tempfile.TemporaryDirectory() as d:
            dev = Device(d)
            dev.init(image, "1.0.0")
            try:
                dev.install(image, "2.0.0", cut_after=n if kind == "slot" else None, cut_state=kind == "state")
            except PowerCut:
                pass
            slot, version, _ = dev.boot()
            assert dev.slot_valid(slot), f"cut at {kind} {n}: booted slot {slot} does not verify"
            if version == "2.0.0":
                slot = dev.confirm()
            results["new" if version == "2.0.0" else "old"] += 1
            # The next boot must work as well.
            slot, version, _ = dev.boot()
            assert dev.slot_valid(slot), f"cut at {kind} {n}: second boot failed"
    print(f"{len(cuts) - 1} power cuts at different points of the update, one update without a cut")
    print(f"booted the old version: {results['old']}, booted the new version: {results['new']}, not bootable: 0")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", default="ab-device")
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("init")
    p.add_argument("image")
    p.add_argument("version")
    p = sub.add_parser("install")
    p.add_argument("image")
    p.add_argument("version")
    p.add_argument("--cut-after", type=int)
    p.add_argument("--cut-state", action="store_true")
    p = sub.add_parser("boot")
    p.add_argument("--bad-app", action="store_true")
    sub.add_parser("confirm")
    sub.add_parser("status")
    p = sub.add_parser("selftest")
    p.add_argument("image")
    args = ap.parse_args()

    if args.cmd == "selftest":
        selftest(args.image)
        return 0
    dev = Device(args.dir)
    if args.cmd == "init":
        dev.init(args.image, args.version)
        print(f"slot a holds {args.version} and is active")
    elif args.cmd == "install":
        try:
            dev.install(args.image, args.version, args.cut_after, args.cut_state)
        except PowerCut as e:
            print(f"POWER CUT: {e}")
            return 3
        print(f"slot {dev.load()['pending']} holds {args.version} and is pending with {TRIES} tries")
    elif args.cmd == "boot":
        slot, version, reason = dev.boot()
        print(f"boot slot {slot} version {version}: {reason}")
        if reason.startswith("trial") and not args.bad_app:
            print("the application passed its self-test")
            print(f"confirmed: slot {dev.confirm()} is active")
        elif reason.startswith("trial"):
            print("the application crashed before it confirmed")
    elif args.cmd == "confirm":
        print(f"confirmed: slot {dev.confirm()} is active")
    elif args.cmd == "status":
        s = dev.load()
        for slot in "ab":
            valid = "valid" if dev.slot_valid(slot) else "not valid"
            mark = " active" if s["active"] == slot else ""
            mark += " pending" if s["pending"] == slot else ""
            print(f"slot {slot}: version {s['versions'][slot]}, {valid}{mark}")
        print(f"tries left: {s['tries']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
