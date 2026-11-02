# sfw

Reads the reports that `tools/ci.sh` leaves in `build/reports`, stores them in a SQLite file and
shows them on a Flask dashboard.

    uv run sfw summary ../../build/reports
    uv run sfw ingest ../../build/reports --db sfw.db
    uv run --extra dashboard sfw serve --db sfw.db
