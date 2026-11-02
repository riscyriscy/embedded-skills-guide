"""A SQLite file with one row for each pipeline run and one row for each test case."""

from __future__ import annotations

import sqlite3

from .reports import Case, Run, Stage

SCHEMA = """
CREATE TABLE IF NOT EXISTS runs (
    id INTEGER PRIMARY KEY,
    version TEXT NOT NULL,
    commit_id TEXT NOT NULL,
    subject TEXT NOT NULL,
    time TEXT NOT NULL,
    line_percent REAL, branch_percent REAL,
    flash INTEGER, ram INTEGER, flash_limit INTEGER, ram_limit INTEGER,
    UNIQUE (commit_id, time)
);
CREATE TABLE IF NOT EXISTS cases (
    run_id INTEGER NOT NULL REFERENCES runs(id) ON DELETE CASCADE,
    suite TEXT NOT NULL, name TEXT NOT NULL, status TEXT NOT NULL,
    seconds REAL NOT NULL, message TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS cases_by_run ON cases (run_id);
CREATE TABLE IF NOT EXISTS stages (
    run_id INTEGER NOT NULL REFERENCES runs(id) ON DELETE CASCADE,
    name TEXT NOT NULL, status TEXT NOT NULL, seconds INTEGER NOT NULL
);
"""

SERIES = {"line_percent", "branch_percent", "flash", "ram"}


def connect(path: str) -> sqlite3.Connection:
    conn = sqlite3.connect(path)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA foreign_keys = ON")
    conn.executescript(SCHEMA)
    return conn


def add_run(conn: sqlite3.Connection, run: Run) -> int:
    """Stores a run and returns its number. The same commit and time gives the number of the stored run."""
    row = conn.execute("SELECT id FROM runs WHERE commit_id = ? AND time = ?", (run.commit, run.time)).fetchone()
    if row:
        return row["id"]
    with conn:
        cur = conn.execute(
            "INSERT INTO runs (version, commit_id, subject, time, line_percent, branch_percent,"
            " flash, ram, flash_limit, ram_limit) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (
                run.version,
                run.commit,
                run.subject,
                run.time,
                run.line_percent,
                run.branch_percent,
                run.flash,
                run.ram,
                run.flash_limit,
                run.ram_limit,
            ),
        )
        conn.executemany(
            "INSERT INTO cases VALUES (?, ?, ?, ?, ?, ?)",
            [(cur.lastrowid, c.suite, c.name, c.status, c.seconds, c.message) for c in run.cases],
        )
        conn.executemany(
            "INSERT INTO stages VALUES (?, ?, ?, ?)",
            [(cur.lastrowid, s.name, s.status, s.seconds) for s in run.stages],
        )
    return int(cur.lastrowid)


def run_row(conn: sqlite3.Connection, run_id: int) -> sqlite3.Row | None:
    return conn.execute(_RUNS + " WHERE r.id = ? GROUP BY r.id", (run_id,)).fetchone()


_RUNS = """
SELECT r.*,
       COUNT(c.name) AS total,
       COALESCE(SUM(c.status = 'pass'), 0) AS passed,
       COALESCE(SUM(c.status = 'fail'), 0) AS failed,
       (SELECT COUNT(*) FROM stages s WHERE s.run_id = r.id AND s.status = 'failed') AS stages_failed
FROM runs r LEFT JOIN cases c ON c.run_id = r.id
"""


def recent_runs(conn: sqlite3.Connection, limit: int = 20) -> list[sqlite3.Row]:
    return conn.execute(_RUNS + " GROUP BY r.id ORDER BY r.id DESC LIMIT ?", (limit,)).fetchall()


def cases_of(conn: sqlite3.Connection, run_id: int) -> list[Case]:
    rows = conn.execute(
        "SELECT suite, name, status, seconds, message FROM cases WHERE run_id = ? ORDER BY suite, name", (run_id,)
    ).fetchall()
    return [Case(**dict(r)) for r in rows]


def stages_of(conn: sqlite3.Connection, run_id: int) -> list[Stage]:
    rows = conn.execute(
        "SELECT name, status, seconds FROM stages WHERE run_id = ? ORDER BY rowid", (run_id,)
    ).fetchall()
    return [Stage(**dict(r)) for r in rows]


def series(conn: sqlite3.Connection, column: str, limit: int = 30) -> list[float]:
    """The values of one column over the newest runs, oldest first. Runs without a value are left out."""
    if column not in SERIES:  # the name goes into the SQL text, so only known names are allowed
        raise ValueError(f"unknown series: {column}")
    rows = conn.execute(
        f"SELECT {column} FROM runs WHERE {column} IS NOT NULL ORDER BY id DESC LIMIT ?", (limit,)
    ).fetchall()
    return [r[0] for r in reversed(rows)]
