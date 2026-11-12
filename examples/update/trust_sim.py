#!/usr/bin/env python3
"""Simulates what a bootloader checks before it trusts an image, and when the device raises its counter.

An image is a header and a payload. The header holds the version, the security counter, the id of the signing key,
the SHA-256 of the payload and the keys that this image revokes. The signature covers the header, as in the MCUboot
format, where the signature covers the hash that the header carries. The device holds the public keys it trusts
(from the factory), the list of revoked keys and the security counter. The counter and the revocations change only when
an image is confirmed, so a trial that fails does not lock the old version out.

Usage: trust_sim.py demo
"""
import base64
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path


def openssl(*args, stdin=None):
    return subprocess.run(["openssl", *args], input=stdin, capture_output=True, check=False)


def make_key(work, name):
    key = work / f"{name}.pem"
    openssl("ecparam", "-genkey", "-name", "prime256v1", "-noout", "-out", str(key))
    openssl("ec", "-in", str(key), "-pubout", "-out", str(work / f"{name}.pub.pem"))
    return key


def sign(work, key, data):
    out = work / "sig.bin"
    openssl("dgst", "-sha256", "-sign", str(key), "-out", str(out), stdin=data)
    return out.read_bytes()


def make_image(work, payload, version, counter, key_id, key, revoke=()):
    header = {"version": version, "counter": counter, "key": key_id, "revoke": list(revoke),
              "sha256": hashlib.sha256(payload).hexdigest()}
    blob = json.dumps(header, sort_keys=True).encode()
    return {"header": header, "payload": base64.b64encode(payload).decode(),
            "signature": base64.b64encode(sign(work, key, blob)).decode()}


class Device:
    def __init__(self, work, trusted, counter):
        self.work, self.trusted, self.counter, self.revoked = work, set(trusted), counter, set()

    def check(self, image):
        """The checks of a bootloader, in order. Returns None when the image passes, else the reason."""
        h = image["header"]
        if h["key"] in self.revoked:
            return f"key {h['key']} is revoked"
        if h["key"] not in self.trusted:
            return f"key {h['key']} is not trusted"
        sig = self.work / "check.sig"
        sig.write_bytes(base64.b64decode(image["signature"]))
        blob = json.dumps(h, sort_keys=True).encode()
        verify = openssl("dgst", "-sha256", "-verify", str(self.work / f"{h['key']}.pub.pem"), "-signature", str(sig), stdin=blob)
        if verify.returncode != 0:
            return "signature does not verify"
        if hashlib.sha256(base64.b64decode(image["payload"])).hexdigest() != h["sha256"]:
            return "payload does not match the signed hash"
        if h["counter"] < self.counter:
            return f"security counter {h['counter']} is below the counter {self.counter} of the device"
        return None

    def confirm(self, image):
        """The application passed its self-test: the counter rises and the revocations take effect."""
        self.counter = max(self.counter, image["header"]["counter"])
        self.revoked |= set(image["header"]["revoke"])


def demo():
    with tempfile.TemporaryDirectory() as tmp:
        work = Path(tmp)
        keys = {name: make_key(work, name) for name in ("k1", "k2", "kx")}
        dev = Device(work, trusted={"k1", "k2"}, counter=1)  # kx belongs to an attacker; k2 is the spare from the factory
        payload = {n: f"firmware {n}".encode() * 20 for n in (1, 2, 3, 4, 5)}

        def offer(label, image, confirm=True, device=None):
            device = device or dev
            reason = device.check(image)
            print(f"{'accepted' if reason is None else 'refused '}  {label}" + (f": {reason}" if reason else ""))
            if reason is None and confirm:
                device.confirm(image)
            return image

        offer("v1, signed by k1, counter 1", make_image(work, payload[1], "1.0.0", 1, "k1", keys["k1"]))
        v2 = offer("v2, signed by k1, counter 2", make_image(work, payload[2], "2.0.0", 2, "k1", keys["k1"]))
        print(f"device counter is {dev.counter}")
        offer("v1 again, a genuine old image", make_image(work, payload[1], "1.0.0", 1, "k1", keys["k1"]))
        bad = json.loads(json.dumps(v2))
        bad["payload"] = base64.b64encode(payload[2][:-1] + b"X").decode()
        offer("v2 with one changed byte", bad)
        bad = json.loads(json.dumps(v2))
        bad["header"]["counter"] = 9
        offer("v2 with the counter edited to 9", bad)
        offer("signed by the attacker key kx, which claims to be k1", make_image(work, payload[3], "3.0.0", 5, "k1", keys["kx"]))
        offer("signed by the attacker key kx, under its own id", make_image(work, payload[3], "3.0.0", 5, "kx", keys["kx"]))
        offer("v3, signed by k2, counter 3, revokes k1", make_image(work, payload[3], "3.0.0", 3, "k2", keys["k2"], revoke=["k1"]))
        print(f"device counter is {dev.counter}, revoked keys: {sorted(dev.revoked)}")
        offer("v4, signed by the leaked key k1, counter 4", make_image(work, payload[4], "4.0.0", 4, "k1", keys["k1"]))
        offer("v5, signed by k2, counter 5, trial that is never confirmed", make_image(work, payload[5], "5.0.0", 5, "k2", keys["k2"]), confirm=False)
        print(f"device counter is {dev.counter}")
        offer("v3 again, after the failed trial of v5", make_image(work, payload[3], "3.0.0", 3, "k2", keys["k2"]))
        # a gap of the rules above: any trusted key may revoke any other key, so a leaked k1 can revoke the spare k2
        print("a second device, before the rotation:")
        other = Device(work, trusted={"k1", "k2"}, counter=1)
        offer("image signed by the leaked key k1, revokes k2", make_image(work, payload[4], "4.0.0", 4, "k1", keys["k1"], revoke=["k2"]), device=other)
        offer("image signed by k2, the spare", make_image(work, payload[5], "5.0.0", 5, "k2", keys["k2"]), device=other)


if __name__ == "__main__":
    if sys.argv[1:] != ["demo"]:
        sys.exit(__doc__)
    demo()
