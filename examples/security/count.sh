#!/bin/bash
# Counts the instructions that valgrind executes in one comparison function, for a
# difference at the first and at the last byte.
# Usage: examples/security/count.sh
set -eu
here=$(cd "$(dirname "$0")" && pwd)
gcc -O1 -g -Wall -Wextra -o /tmp/compare "$here/compare.c"
for fn in leaky ct; do
    for pos in 0 15; do
        n=$(valgrind --tool=callgrind --callgrind-out-file=/dev/null \
            --toggle-collect="${fn}_equal" /tmp/compare "$fn" "$pos" 2>&1 \
            | sed -n 's/^==[0-9]*== Collected : //p')
        printf '%-6s difference at byte %2d: %5s instructions\n' "$fn" "$pos" "$n"
    done
done
