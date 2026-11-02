import json
from pathlib import Path

import pytest

JUNIT = """<?xml version="1.0" encoding="UTF-8"?>
<testsuite name="(empty)" tests="2" failures="{failures}">
  <testcase name="test_ringbuf" classname="test_ringbuf" time="0.003" status="run">
    <system-out>./tests/test_ringbuf.c:90:test_new_buffer_is_empty:PASS
./tests/test_ringbuf.c:91:test_mean:{mean}
-----------------------
2 Tests {failures} Failures 0 Ignored
</system-out>
  </testcase>
  <testcase name="test_sampler" classname="test_sampler" time="0.002" status="run"/>
</testsuite>
"""


@pytest.fixture
def reports(tmp_path: Path) -> Path:
    """A reports directory as tools/ci.sh leaves it, with all tests passing."""
    write_reports(tmp_path)
    return tmp_path


def write_reports(
    directory: Path, failing: bool = False, commit: str = "a" * 40, flash: int = 468, gate_failed: bool = False
) -> Path:
    directory.mkdir(parents=True, exist_ok=True)
    (directory / "stage-host.json").write_text(
        json.dumps({"name": "host", "status": "failed" if failing else "passed", "seconds": 1})
    )
    (directory / "stage-coverage.json").write_text(
        json.dumps({"name": "coverage", "status": "failed" if gate_failed else "passed", "seconds": 2})
    )
    (directory / "junit-host.xml").write_text(
        JUNIT.format(failures=1 if failing else 0, mean="FAIL:Expected 8 Was 7" if failing else "PASS")
    )
    (directory / "meta.json").write_text(
        json.dumps(
            {"version": "v0.1.0-3-gabcdef0", "commit": commit, "subject": "A change", "time": "2026-11-02T10:00:00Z"}
        )
    )
    (directory / "coverage.json").write_text(json.dumps({"line_percent": 98.9, "branch_percent": 94.1}))
    (directory / "size.json").write_text(
        json.dumps({"flash": flash, "ram": 0, "flash_limit": 262144, "ram_limit": 65536})
    )
    return directory
