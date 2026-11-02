import pytest

from sfw.reports import load_run
from sfw.web import create_app, sparkline

from .conftest import write_reports


@pytest.fixture
def client(tmp_path):
    app = create_app(str(tmp_path / "sfw.db"), token="secret")
    return app.test_client()


def publish(client, directory, token="secret"):
    headers = {"Authorization": f"Bearer {token}"} if token else {}
    return client.post("/api/runs", json=load_run(directory).to_dict(), headers=headers)


def test_publish_then_read(client, reports):
    response = publish(client, reports)
    assert response.status_code == 201
    run_id = response.get_json()["run"]
    listing = client.get("/api/runs").get_json()
    assert [r["id"] for r in listing] == [run_id]
    page = client.get(f"/runs/{run_id}")
    assert page.status_code == 200 and b"test_new_buffer_is_empty" in page.data


@pytest.mark.parametrize("token, code", [("wrong", 401), (None, 401)])
def test_publish_needs_the_token(client, reports, token, code):
    assert publish(client, reports, token).status_code == code


def test_a_server_without_a_token_refuses_to_publish(tmp_path, reports):
    app = create_app(str(tmp_path / "x.db"), token="")
    assert publish(app.test_client(), reports).status_code == 503


def test_a_body_that_is_not_a_run_is_rejected(client):
    r = client.post("/api/runs", json={"version": "v1"}, headers={"Authorization": "Bearer secret"})
    assert r.status_code == 400


def test_text_from_a_commit_is_escaped(client, tmp_path):
    directory = write_reports(tmp_path)
    run = load_run(directory)
    run.subject = "<script>alert(1)</script>"
    client.post("/api/runs", json=run.to_dict(), headers={"Authorization": "Bearer secret"})
    page = client.get("/").data
    assert b"<script>alert(1)</script>" not in page and b"&lt;script&gt;" in page


def test_a_failed_gate_shows_on_the_pages(client, tmp_path):
    response = publish(client, write_reports(tmp_path, gate_failed=True))
    run_id = response.get_json()["run"]
    assert b'badge fail">fail' in client.get("/").data
    assert b"coverage" in client.get(f"/runs/{run_id}").data


def test_the_error_names_the_missing_fields(client):
    r = client.post("/api/runs", json={"version": "v1"}, headers={"Authorization": "Bearer secret"})
    assert r.get_json() == {"error": "not a run: missing commit, subject, time"}


def test_an_unknown_field_is_rejected(client):
    body = {"version": "v1", "commit": "c", "subject": "s", "time": "t", "colour": "red"}
    r = client.post("/api/runs", json=body, headers={"Authorization": "Bearer secret"})
    assert r.status_code == 400


def test_unknown_run_is_a_404(client):
    assert client.get("/runs/99").status_code == 404


def test_sparkline():
    assert sparkline([]) == ""
    assert sparkline([5.0]) == "0.0,18.0"
    assert sparkline([1, 3]).split()[0].endswith(",32.0")
