#!/bin/bash
# Compiles each snippet in snippets/ under four C++ standards with the target compiler and prints a
# table: ok when the compiler and its library accept it, error when they do not.
# Usage: examples/modern-cpp/support.sh [compiler]
set -u
cd "$(dirname "$0")/snippets" || exit 1
cxx=${1:-arm-none-eabi-g++}
stds=(c++14 c++17 c++20 c++23)
printf '%-30s' feature
printf ' %-6s' "${stds[@]}"
printf '\n'
for f in cpp17_*.cpp cpp20_*.cpp cpp23_*.cpp; do
    printf '%-30s' "${f%.cpp}"
    for s in "${stds[@]}"; do
        if "$cxx" -std="$s" -pedantic-errors -fsyntax-only "$f" > /dev/null 2>&1; then
            printf ' %-6s' ok
        else
            printf ' %-6s' error
        fi
    done
    printf '\n'
done
