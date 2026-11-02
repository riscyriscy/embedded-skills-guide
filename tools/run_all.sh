#!/bin/bash
# Runs the host and the Cortex-M4 workflows at the same time and reports both results.
# The log of each run goes to build/logs. Exit status 1 when any run fails.
set -u
cd "$(git rev-parse --show-toplevel)" || exit 1
mkdir -p build/logs

presets=(ci-host ci-m4)
declare -A pid
for p in "${presets[@]}"; do
    cmake --workflow --preset "$p" > "build/logs/$p.log" 2>&1 &
    pid[$p]=$!
done

failed=0
for p in "${presets[@]}"; do
    if wait "${pid[$p]}"; then
        echo "$p: passed"
    else
        echo "$p: FAILED, last lines of build/logs/$p.log:"
        tail -n 5 "build/logs/$p.log" | sed 's/^/    /'
        failed=1
    fi
done
exit $failed
