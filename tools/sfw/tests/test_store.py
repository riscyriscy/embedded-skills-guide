import pytest

from sfw import store
from sfw.reports import load_run

from .conftest import write_reports


def test_the_same_run_is_stored_once(reports):
    conn = store.connect(":memory:")
    first = store.add_run(conn, load_run(reports))
    again = store.add_run(conn, load_run(reports))
    assert first == again
    assert conn.execute("SELECT COUNT(*) FROM runs").fetchone()[0] == 1


def test_counts_and_series(tmp_path):
    conn = store.connect(":memory:")
    store.add_run(conn, load_run(write_reports(tmp_path / "a", commit="1" * 40, flash=400)))
    store.add_run(conn, load_run(write_reports(tmp_path / "b", failing=True, commit="2" * 40, flash=500)))
    newest, oldest = store.recent_runs(conn)
    assert (newest["total"], newest["failed"]) == (3, 1)
    assert (oldest["total"], oldest["failed"]) == (3, 0)
    assert store.series(conn, "flash") == [400, 500]


def test_only_known_series_names_reach_the_sql():
    conn = store.connect(":memory:")
    with pytest.raises(ValueError):
        store.series(conn, "flash; DROP TABLE runs")


def test_stages_are_stored_with_the_run(tmp_path):
    conn = store.connect(":memory:")
    run_id = store.add_run(conn, load_run(write_reports(tmp_path, gate_failed=True)))
    assert [(s.name, s.status) for s in store.stages_of(conn, run_id)] == [("coverage", "failed"), ("host", "passed")]
    assert store.recent_runs(conn)[0]["stages_failed"] == 1
