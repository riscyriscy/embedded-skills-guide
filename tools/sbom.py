#!/usr/bin/env python3
"""Writes a CycloneDX bill of materials for one image, from the archives that the linker kept.

Usage: tools/sbom.py <binary> <map file> <version> <output>

Every archive that the map names must have a row in tools/sbom-parts.tsv. Without one the script
stops and names the archive, because a release must not carry a list of parts with an entry that
nobody has identified. The time and the serial number come from the commit, so every build of that
commit writes the same file, as Chapter 11 of Volume 1 asks.
"""

import hashlib
import json
import pathlib
import re
import subprocess
import sys
import uuid

ARCHIVE = re.compile(r"^(\S+\.a)\(")
REPOSITORY = "https://github.com/riscyriscy/embedded-skills-guide"


def git(*args):
    out = subprocess.run(["git", *args], capture_output=True, text=True, check=True)
    return out.stdout.strip()


def archives(map_file):
    """The archives of the block "Archive member included to satisfy reference by file (symbol)"."""
    lines = pathlib.Path(map_file).read_text().splitlines()
    start = next(i for i, line in enumerate(lines) if line.startswith("Archive member included")) + 1
    found = []
    for line in lines[start:]:
        if ARCHIVE.match(line):
            found.append(pathlib.Path(ARCHIVE.match(line).group(1)).name)
        elif line and not line.startswith(" "):
            break
    return sorted(set(found))


def parts():
    """tools/sbom-parts.tsv: archive, component, version, supplier, licence."""
    table = {}
    for line in pathlib.Path("tools/sbom-parts.tsv").read_text().splitlines():
        if line.startswith("#") or not line.strip():
            continue
        archive, name, version, supplier, licence = (f.strip() for f in line.split("\t") if f.strip())
        table[archive] = (name, version, supplier, licence)
    return table


def spdx(licence):
    """An SPDX identifier goes in "id"; anything else, such as a licence of several parts, in "name"."""
    return {"id": licence} if re.fullmatch(r"[A-Za-z0-9.+-]+", licence) else {"name": licence}


def main(argv):
    binary, map_file, version, output = argv[1:5]
    table = parts()
    missing = [a for a in archives(map_file) if a not in table]
    if missing:
        sys.exit("sbom.py: no row in tools/sbom-parts.tsv for " + ", ".join(missing))
    commit = git("rev-parse", "HEAD")
    compiler = subprocess.run(["arm-none-eabi-gcc", "-dumpversion"], capture_output=True, text=True,
                              check=True).stdout.strip()
    components = []
    for archive in archives(map_file):
        name, part_version, supplier, licence = table[archive]
        components.append({
            "type": "library",
            "bom-ref": f"{name}@{part_version if part_version != 'project' else version}",
            "name": name,
            "version": version if part_version == "project" else part_version,
            "supplier": {"name": supplier},
            "licenses": [{"license": spdx(licence)}],
            "properties": [{"name": "sensorfw:archive", "value": archive}],
        })
    product = {
        "type": "firmware",
        "bom-ref": f"sensorfw@{version}",
        "name": "sensorfw",
        "version": version,
        "supplier": {"name": "sensorfw"},
        "licenses": [{"license": {"id": "MIT"}}],
        "hashes": [{"alg": "SHA-256", "content": hashlib.sha256(pathlib.Path(binary).read_bytes()).hexdigest()}],
        "externalReferences": [{"type": "vcs", "url": REPOSITORY, "comment": commit}],
    }
    bom = {
        "bomFormat": "CycloneDX",
        "specVersion": "1.6",
        "serialNumber": "urn:uuid:" + str(uuid.uuid5(uuid.NAMESPACE_URL, f"urn:sensorfw:{commit}")),
        "version": 1,
        "metadata": {
            "timestamp": git("log", "-1", "--format=%cI"),
            "tools": {"components": [{"type": "application", "name": "arm-none-eabi-gcc", "version": compiler}]},
            "component": product,
        },
        "components": components,
        "dependencies": [{"ref": product["bom-ref"], "dependsOn": [c["bom-ref"] for c in components]}],
    }
    pathlib.Path(output).write_text(json.dumps(bom, indent=2) + "\n")
    print(f"sbom: {len(components)} parts in {output}")


if __name__ == "__main__":
    main(sys.argv)
