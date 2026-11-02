"""The dashboard: a Flask application over the SQLite file of sfw."""

from __future__ import annotations

import hmac
import os
import sqlite3

from flask import Flask, abort, g, jsonify, render_template, request, url_for

from . import store
from .reports import Run


def sparkline(values: list[float], width: int = 160, height: int = 36) -> str:
    """Points for an SVG polyline. One value gives a flat line, no value gives an empty string."""
    if not values:
        return ""
    low, high = min(values), max(values)
    step = width / max(len(values) - 1, 1)

    def y(v: float) -> float:
        if high == low:  # a series that does not change is drawn in the middle
            return height / 2
        return height - 4 - (v - low) / (high - low) * (height - 8)

    return " ".join(f"{i * step:.1f},{y(v):.1f}" for i, v in enumerate(values))


def create_app(db_path: str | None = None, token: str | None = None) -> Flask:
    app = Flask(__name__)
    app.config.update(
        DB=db_path or os.environ.get("SFW_DB", "sfw.db"),
        TOKEN=token if token is not None else os.environ.get("SFW_TOKEN", ""),
        MAX_CONTENT_LENGTH=1024 * 1024,  # a run with a few thousand cases is far below this
    )

    def db() -> sqlite3.Connection:
        if "db" not in g:
            g.db = store.connect(app.config["DB"])
        return g.db

    @app.teardown_appcontext
    def close_db(_exc: BaseException | None) -> None:
        conn = g.pop("db", None)
        if conn is not None:
            conn.close()

    @app.get("/")
    def index():
        runs = store.recent_runs(db(), 20)
        charts = [
            ("Line coverage", "line_percent", "%"),
            ("Branch coverage", "branch_percent", "%"),
            ("Flash", "flash", " bytes"),
            ("RAM", "ram", " bytes"),
        ]
        trends = []
        for title, column, unit in charts:
            values = store.series(db(), column)
            trends.append(
                {"title": title, "points": sparkline(values), "last": values[-1] if values else None, "unit": unit}
            )
        return render_template("index.html", runs=runs, trends=trends)

    @app.get("/runs/<int:run_id>")
    def show_run(run_id: int):
        row = store.run_row(db(), run_id)
        if row is None or row["id"] is None:
            abort(404)
        cases = store.cases_of(db(), run_id)
        cases.sort(key=lambda c: (c.status != "fail", c.suite, c.name))
        return render_template("run.html", run=row, cases=cases, stages=store.stages_of(db(), run_id))

    @app.get("/api/runs")
    def api_runs():
        rows = store.recent_runs(db(), request.args.get("limit", 20, type=int))
        return jsonify([{k: r[k] for k in r.keys()} for r in rows])

    @app.post("/api/runs")
    def api_publish():
        expected = app.config["TOKEN"]
        if not expected:
            return jsonify(error="publishing is off: the server has no token"), 503
        sent = request.headers.get("Authorization", "").removeprefix("Bearer ")
        if not hmac.compare_digest(sent.encode(), expected.encode()):
            return jsonify(error="wrong or missing token"), 401
        data = request.get_json(silent=True)
        if not isinstance(data, dict):
            return jsonify(error="the body must be a JSON object"), 400
        try:
            run = Run.from_dict(data)
        except ValueError as error:
            return jsonify(error=f"not a run: {error}"), 400
        run_id = store.add_run(db(), run)
        return jsonify(run=run_id, url=url_for("show_run", run_id=run_id, _external=True)), 201

    @app.get("/healthz")
    def healthz():
        db().execute("SELECT 1")
        return "ok\n"

    return app
