#!/usr/bin/env python3
"""The factory tool. "make" writes the record of one unit, "show" reads one back.

The log keeps one line for every unit that was made. A serial number, a MAC address or a key that is
already in the log is refused, because each of the three must be unique across every unit shipped.
"""
import argparse
import csv
import datetime
import hashlib
import os
import struct
import subprocess
import sys
import zlib

MAGIC = 0x54494E55  # the bytes "UNIT"
VERSION = 1
LAYOUT = "<IHH16s6sBBiI8sI"  # the same 52 bytes as unit_record.h
SIZE = struct.calcsize(LAYOUT)
LOG_FIELDS = ["date", "serial", "mac", "key_id", "hw_rev", "image"]


def key_id(pubkey_pem):
    """The first 8 bytes of the SHA-256 of the public key in DER form."""
    der = subprocess.run(["openssl", "pkey", "-pubin", "-in", pubkey_pem, "-outform", "DER"],
                         check=True, capture_output=True).stdout
    return hashlib.sha256(der).digest()[:8]


def parse_mac(text):
    octets = bytes(int(part, 16) for part in text.split(":"))
    if len(octets) != 6:
        sys.exit(f"provision.py: a MAC address has six octets: {text}")
    return octets


def build(serial, mac, hw_rev, offset, gain_ppm, key):
    if len(serial) > 15:
        sys.exit("provision.py: the serial number is longer than 15 characters")
    fields = (MAGIC, VERSION, SIZE, serial.encode(), mac, hw_rev, 0, offset, gain_ppm, key)
    body = struct.pack(LAYOUT[:-1], *fields)
    return body + struct.pack("<I", zlib.crc32(body))


def unpack(blob):
    unit = struct.unpack(LAYOUT, blob[:SIZE])
    magic, version, length, serial, mac, hw_rev, _, offset, gain, key, crc = unit
    ok = magic == MAGIC and version == VERSION and length == SIZE and crc == zlib.crc32(blob[: SIZE - 4])
    return ok, serial.rstrip(b"\0").decode(), mac, hw_rev, offset, gain, key


def read_log(path):
    if not os.path.exists(path):
        return []
    with open(path, newline="") as f:
        return list(csv.DictReader(f))


def make(args):
    mac = parse_mac(args.mac)
    key = key_id(args.pubkey)
    rows = read_log(args.log)
    for field, value in (("serial", args.serial), ("mac", args.mac), ("key_id", key.hex())):
        if any(row[field] == value for row in rows):
            sys.exit(f"provision.py: {field} {value} is already in {args.log}")
    blob = build(args.serial, mac, args.hw_rev, args.offset, args.gain, key)
    with open(args.out, "wb") as f:
        f.write(blob)
    new = not rows
    with open(args.log, "a", newline="") as f:
        writer = csv.DictWriter(f, LOG_FIELDS)
        if new:
            writer.writeheader()
        today = datetime.datetime.now(tz=datetime.UTC).date().isoformat()
        writer.writerow({"date": today, "serial": args.serial, "mac": args.mac, "key_id": key.hex(),
                         "hw_rev": args.hw_rev, "image": args.image})
    crc = struct.unpack("<I", blob[-4:])[0]
    print(f"{args.out}: {len(blob)} bytes, crc32 {crc:08x}")


def show(args):
    with open(args.record, "rb") as f:
        ok, serial, mac, hw_rev, offset, gain_ppm, key = unpack(f.read())
    address = ":".join(f"{b:02x}" for b in mac)
    state = "valid" if ok else "INVALID"
    print(f"{serial}  mac {address}  rev {hw_rev}  offset {offset}  gain {gain_ppm} ppm  "
          f"key {key.hex()}  {state}")
    sys.exit(0 if ok else 1)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    m = sub.add_parser("make", help="write the record of one unit")
    m.add_argument("--serial", required=True)
    m.add_argument("--mac", required=True, help="six octets, separated by colons")
    m.add_argument("--hw-rev", type=int, required=True)
    m.add_argument("--offset", type=int, required=True, help="calibration offset in counts")
    m.add_argument("--gain", type=int, required=True, help="calibration gain in parts per million")
    m.add_argument("--pubkey", required=True, help="public key of the unit, PEM")
    m.add_argument("--image", default="", help="version of the image that is programmed, for the log")
    m.add_argument("--log", default="factory.csv")
    m.add_argument("--out", required=True)
    m.set_defaults(func=make)
    s = sub.add_parser("show", help="read a record back")
    s.add_argument("record")
    s.set_defaults(func=show)
    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
