#!/bin/bash
# Runs a command many times and counts the failures, to measure a test that fails now and then.
# Usage: tools/repeat.sh <runs> <command> [arguments]
# Prints one line with the counts and, when a run failed, the output of the first failure.
# Exit status 0 when every run passed and 1 when any run failed.
set -u

runs=${1:?usage: repeat.sh <runs> <command> [arguments]}
shift
[ "$#" -gt 0 ] || { echo "repeat.sh: no command" >&2; exit 2; }

failed=0
first=0
first_output=
for ((run = 1; run <= runs; run++)); do
    if ! output=$("$@" 2>&1); then
        failed=$((failed + 1))
        if [ "$first" -eq 0 ]; then
            first=$run
            first_output=$output
        fi
    fi
done

awk -v runs="$runs" -v failed="$failed" 'BEGIN {
    printf "passed %d of %d runs, failed %d (%.1f%%)\n", runs - failed, runs, failed, 100 * failed / runs
}'
if [ "$failed" -gt 0 ]; then
    printf 'first failure, run %d:\n%s\n' "$first" "$first_output"
    exit 1
fi
