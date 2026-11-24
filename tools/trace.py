#!/usr/bin/env python3
"""Reports which requirements have a test.

A requirement list is a CSV file with the columns id and text. A test names the
requirement it checks in a comment with the id (REQ-012) next to the test function.
The tool prints one row for each requirement and exits with status 1 when a
requirement has no test or a comment names an id that the list does not have.

Usage: tools/trace.py <requirements.csv> <directory with tests> [more directories]
"""
import csv
import pathlib
import re
import sys

TAG = re.compile(r"\bREQ-\d+\b")
SUFFIXES = {".c", ".h", ".cpp", ".py"}


def find_tags(roots):
    """Maps each id to the list of file:line places that name it."""
    found = {}
    for root in roots:
        for path in sorted(pathlib.Path(root).rglob("*")):
            if path.suffix not in SUFFIXES or not path.is_file():
                continue
            text = path.read_text(errors="replace")
            for number, line in enumerate(text.splitlines(), 1):
                for tag in TAG.findall(line):
                    found.setdefault(tag, []).append(f"{path.name}:{number}")
    return found


def main(argv):
    if len(argv) < 3:
        print(__doc__.strip().splitlines()[-1], file=sys.stderr)
        return 2
    with open(argv[1], newline="") as f:
        reqs = {row["id"]: row["text"] for row in csv.DictReader(f)}
    found = find_tags(argv[2:])

    status = 0
    for rid, text in reqs.items():
        where = found.get(rid, [])
        if not where:
            status = 1
        label = "covered" if where else "NO TEST"
        print(f"{rid}  {label:8}  {', '.join(where) or text}")
    for rid in sorted(set(found) - set(reqs)):
        status = 1
        print(f"{rid}  UNKNOWN   named in {', '.join(found[rid])}, not in the list")
    covered = sum(1 for rid in reqs if rid in found)
    print(f"{covered} of {len(reqs)} requirements have a test")
    return status


if __name__ == "__main__":
    sys.exit(main(sys.argv))
