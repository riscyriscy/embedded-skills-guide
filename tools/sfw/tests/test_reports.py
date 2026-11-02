from sfw.reports import load_run, parse_junit

from .conftest import write_reports


def test_unity_lines_become_cases(reports):
    cases = parse_junit(reports / "junit-host.xml")
    names = [(c.name, c.status) for c in cases]
    assert ("test_ringbuf.test_new_buffer_is_empty", "pass") in names
    # A program without Unity lines is one case of its own.
    assert ("test_sampler", "pass") in names


def test_a_failing_unity_test_keeps_its_message(tmp_path):
    write_reports(tmp_path, failing=True)
    failed = [c for c in parse_junit(tmp_path / "junit-host.xml") if c.status == "fail"]
    assert [(c.name, c.message) for c in failed] == [("test_ringbuf.test_mean", "Expected 8 Was 7")]


def test_load_run_reads_every_file(reports):
    run = load_run(reports)
    assert run.version == "v0.1.0-3-gabcdef0"
    assert (run.line_percent, run.branch_percent) == (98.9, 94.1)
    assert (run.flash, run.ram_limit) == (468, 65536)
    assert run.status == "pass"


def test_a_run_without_reports_still_loads(tmp_path):
    write_reports(tmp_path)
    (tmp_path / "coverage.json").unlink()
    (tmp_path / "size.json").unlink()
    run = load_run(tmp_path)
    assert run.line_percent is None and run.flash is None


def test_a_failed_gate_fails_the_run_without_a_failed_test(tmp_path):
    write_reports(tmp_path, gate_failed=True)
    run = load_run(tmp_path)
    assert run.failed == 0 and run.stages_failed == 1
    assert run.status == "fail"
