#!/bin/bash
# Makes a small git history with one tag, to try tools/changelog.sh on. Usage: demo_history.sh <empty directory>
set -eu
dir=${1:?usage: demo_history.sh <directory>}
mkdir -p "$dir"
cd "$dir"
git init -q
git config user.name "Demo"
git config user.email "demo@example.com"
commit() { echo "$1" >> log.txt; git add log.txt; git commit -q -m "$1" ${2:+-m "$2"}; }
commit "feat: ring buffer with fixed capacity"
commit "fix: stats_mean divided by zero on an empty buffer"
git tag v1.2.3
commit "fix: reject samples above 4095 counts"
commit "perf: sum the samples without a call for each one"
commit "docs: describe the units of the sample"
commit "feat: add stats_min and stats_max"
commit "test: cover the empty buffer"
