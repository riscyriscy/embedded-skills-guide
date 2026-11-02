#!/bin/bash
# Compiles each snippet in snippets/ under four C standards with the target compiler and prints
# a table: ok when the compiler accepts it, error when it does not. -pedantic-errors turns every
# use of an extension into an error, so ok means that the feature is part of the standard in use.
# Usage: examples/modern-c/support.sh [compiler]
set -u
cd "$(dirname "$0")/snippets" || exit 1
cc=${1:-arm-none-eabi-gcc}
stds=(c99 c11 c17 c2x)
printf '%-30s' feature
printf ' %-6s' "${stds[@]}"
printf '\n'
for f in c99_*.c c11_*.c c23_*.c; do
    printf '%-30s' "${f%.c}"
    for s in "${stds[@]}"; do
        if "$cc" -std="$s" -pedantic-errors -fsyntax-only "$f" > /dev/null 2>&1; then
            printf ' %-6s' ok
        else
            printf ' %-6s' error
        fi
    done
    printf '\n'
done
