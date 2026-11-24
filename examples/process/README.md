# examples/process

Data for Chapter 19 of Volume 1 of the Embedded Skills Guide, reviews, versions and requirements.

| File | Purpose |
|---|---|
| `demo_history.sh` | makes a small git history with the tag v1.2.3, to try `tools/changelog.sh` on |
| `trace-demo/requirements.csv` | six requirements with identifiers REQ-001 to REQ-006 |
| `trace-demo/tests/` | two test programs whose functions carry a `REQ-nnn` comment |

Run: `tools/trace.py examples/process/trace-demo/requirements.csv examples/process/trace-demo/tests`
