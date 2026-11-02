"""Reads the files that tools/ci.sh writes to build/reports into one Run."""

from __future__ import annotations

import json
import re
import xml.etree.ElementTree as ET
from dataclasses import asdict, dataclass, field
from pathlib import Path

# A line of Unity output: ./tests/test_frame.c:127:test_name:PASS   or   ...:FAIL:message
UNITY = re.compile(r"^(?P<file>[^:\s]+):(?P<line>\d+):(?P<name>\w+):(?P<state>PASS|FAIL|IGNORE)(?::(?P<message>.*))?$")


@dataclass
class Case:
    suite: str
    name: str
    status: str  # pass, fail or skip
    seconds: float = 0.0
    message: str = ""


@dataclass
class Stage:
    name: str
    status: str  # passed or failed
    seconds: int = 0


@dataclass
class Run:
    version: str
    commit: str
    subject: str
    time: str
    cases: list[Case] = field(default_factory=list)
    line_percent: float | None = None
    branch_percent: float | None = None
    flash: int | None = None
    ram: int | None = None
    flash_limit: int | None = None
    ram_limit: int | None = None
    stages: list[Stage] = field(default_factory=list)

    @property
    def failed(self) -> int:
        return sum(1 for c in self.cases if c.status == "fail")

    @property
    def passed(self) -> int:
        return sum(1 for c in self.cases if c.status == "pass")

    @property
    def stages_failed(self) -> int:
        return sum(1 for s in self.stages if s.status == "failed")

    @property
    def status(self) -> str:
        """A run fails when a test fails and also when a stage fails without a failed test, such as a coverage gate."""
        return "fail" if self.failed or self.stages_failed else "pass"

    def to_dict(self) -> dict:
        return asdict(self)

    @classmethod
    def from_dict(cls, data: dict) -> Run:
        missing = [k for k in ("version", "commit", "subject", "time") if k not in data]
        if missing:
            raise ValueError("missing " + ", ".join(missing))
        try:
            cases = [Case(**c) for c in data.get("cases", [])]
            stages = [Stage(**s) for s in data.get("stages", [])]
            return cls(**{**data, "cases": cases, "stages": stages})
        except TypeError as error:  # an unknown or missing field, or an entry that is not an object
            raise ValueError("a field is unknown or missing in the run, its cases or its stages") from error


def parse_junit(path: Path) -> list[Case]:
    """The cases of one JUnit file written by CTest.

    CTest writes one testcase for each test program and puts the output of the program in system-out.
    When that output holds Unity lines, each Unity test becomes a case of its own. This is how a
    pipeline sees which test of a program failed and not only which program.
    """
    suite = path.stem.removeprefix("junit-")
    cases: list[Case] = []
    for tc in ET.parse(path).getroot().iter("testcase"):
        program = tc.get("name", "?")
        seconds = float(tc.get("time", "0") or 0)
        output = tc.findtext("system-out") or ""
        unity = [m for line in output.splitlines() if (m := UNITY.match(line.strip()))]
        if unity:
            for m in unity:
                state = {"PASS": "pass", "FAIL": "fail", "IGNORE": "skip"}[m["state"]]
                cases.append(Case(suite, f"{program}.{m['name']}", state, 0.0, (m["message"] or "").strip()))
        else:
            failure = tc.find("failure")
            if failure is not None:
                cases.append(Case(suite, program, "fail", seconds, failure.get("message", "")))
            elif tc.get("status") == "notrun":
                cases.append(Case(suite, program, "skip", seconds))
            else:
                cases.append(Case(suite, program, "pass", seconds))
    return cases


def load_run(directory: Path) -> Run:
    """Reads build/reports. Only meta.json is required: a pipeline that stopped early still has a run."""
    meta = json.loads((directory / "meta.json").read_text())
    run = Run(meta["version"], meta["commit"], meta["subject"], meta["time"])
    for xml in sorted(directory.glob("junit-*.xml")):
        run.cases += parse_junit(xml)
    for stage in sorted(directory.glob("stage-*.json")):
        run.stages.append(Stage(**json.loads(stage.read_text())))
    coverage = directory / "coverage.json"
    if coverage.exists():
        summary = json.loads(coverage.read_text())
        run.line_percent = summary.get("line_percent")
        run.branch_percent = summary.get("branch_percent")
    size = directory / "size.json"
    if size.exists():
        sizes = json.loads(size.read_text())
        run.flash, run.ram = sizes["flash"], sizes["ram"]
        run.flash_limit, run.ram_limit = sizes["flash_limit"], sizes["ram_limit"]
    return run
