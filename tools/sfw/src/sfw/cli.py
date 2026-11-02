"""The command line of sfw: summary, ingest, runs, publish and serve."""

from __future__ import annotations

import argparse
import json
import os
import sys
import urllib.error
import urllib.request
from pathlib import Path

from . import store
from .reports import load_run


def cmd_summary(args: argparse.Namespace) -> int:
    run = load_run(Path(args.reports))
    print(f"{run.version}  {run.subject}")
    suites: dict[str, list[str]] = {}
    for case in run.cases:
        suites.setdefault(case.suite, []).append(case.status)
    for suite, states in suites.items():
        counts = f"{states.count('pass'):>3} passed {states.count('fail'):>3} failed {states.count('skip'):>3} skipped"
        print(f"  {suite:<9} {counts}")
    if run.line_percent is not None:
        print(f"  coverage  lines {run.line_percent:.1f}%  branches {run.branch_percent:.1f}%")
    if run.flash is not None:
        print(f"  size      flash {run.flash} of {run.flash_limit} bytes  ram {run.ram} of {run.ram_limit} bytes")
    for case in run.cases:
        if case.status == "fail":
            print(f"  FAILED    {case.suite}: {case.name} {case.message}".rstrip())
    for stage in run.stages:
        if stage.status == "failed":
            print(f"  FAILED    stage {stage.name}")
    return 1 if run.status == "fail" else 0


def cmd_ingest(args: argparse.Namespace) -> int:
    run = load_run(Path(args.reports))
    with store.connect(args.db) as conn:
        run_id = store.add_run(conn, run)
    print(f"run {run_id}: {run.version}, {len(run.cases)} cases, {run.failed} failed")
    return 0


def cmd_runs(args: argparse.Namespace) -> int:
    conn = store.connect(args.db)
    for r in reversed(store.recent_runs(conn, args.limit)):
        coverage = "    -" if r["line_percent"] is None else f"{r['line_percent']:5.1f}%"
        flash = "    -" if r["flash"] is None else f"{r['flash']:5d}"
        result = "FAIL" if r["failed"] or r["stages_failed"] else "ok  "
        tests = f"{r['passed']:>3}/{r['total']:<3}"
        print(f"{r['id']:>3}  {r['version']:<22} {tests} {result}  lines {coverage}  flash {flash}")
    return 0


def cmd_publish(args: argparse.Namespace) -> int:
    """Sends a run to the dashboard. The token comes from the environment of the pipeline, not from the command line."""
    if not args.token:
        print("publish: no token: set SFW_TOKEN or pass --token", file=sys.stderr)
        return 2
    run = load_run(Path(args.reports))
    request = urllib.request.Request(
        args.url.rstrip("/") + "/api/runs",
        data=json.dumps(run.to_dict()).encode(),
        headers={"Content-Type": "application/json", "Authorization": f"Bearer {args.token}"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(request, timeout=10) as response:
            print(response.read().decode().strip())
    except urllib.error.HTTPError as error:
        print(f"publish: the dashboard answered {error.code}: {error.read().decode().strip()}", file=sys.stderr)
        return 1
    except urllib.error.URLError as error:
        print(f"publish: no answer from {args.url}: {error.reason}", file=sys.stderr)
        return 1
    return 0


def cmd_serve(args: argparse.Namespace) -> int:
    try:
        from .web import create_app
    except ImportError:
        print("serve needs Flask: run it with  uv run --extra dashboard sfw serve", file=sys.stderr)
        return 2
    create_app(args.db).run(host=args.host, port=args.port)
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="sfw", description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("summary", help="print the results in a reports directory")
    p.add_argument("reports")
    p.set_defaults(func=cmd_summary)

    p = sub.add_parser("ingest", help="store a reports directory in the database")
    p.add_argument("reports")
    p.add_argument("--db", default="sfw.db")
    p.set_defaults(func=cmd_ingest)

    p = sub.add_parser("runs", help="list the stored runs")
    p.add_argument("--db", default="sfw.db")
    p.add_argument("--limit", type=int, default=20)
    p.set_defaults(func=cmd_runs)

    p = sub.add_parser("publish", help="send a reports directory to a dashboard")
    p.add_argument("reports")
    p.add_argument("--url", required=True)
    p.add_argument("--token", default=os.environ.get("SFW_TOKEN"), help="default: the variable SFW_TOKEN")
    p.set_defaults(func=cmd_publish)

    p = sub.add_parser("serve", help="run the dashboard for development")
    p.add_argument("--db", default="sfw.db")
    p.add_argument("--host", default="127.0.0.1")
    p.add_argument("--port", type=int, default=5000)
    p.set_defaults(func=cmd_serve)

    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
