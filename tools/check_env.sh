#!/bin/bash
# Checks that the tools of the build are installed and new enough.
# Prints one line per tool. Exit status 1 means a tool is missing or too old.
set -u

# name, command that prints the version, minimum version
tools=(
    "cmake|cmake --version|3.25"
    "ninja|ninja --version|1.10"
    "make|make --version|4.0"
    "git|git --version|2.30"
    "gcc|gcc -dumpfullversion|11"
    "arm-none-eabi-gcc|arm-none-eabi-gcc -dumpfullversion|12"
    "qemu-system-arm|qemu-system-arm --version|7.0"
    "clang-format|clang-format --version|14"
)

# Prints the first thing that looks like a version number in the text on stdin.
first_version() { grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n 1; }

# True when version $1 is at least version $2, comparing numerically field by field.
at_least() { [ "$(printf '%s\n%s\n' "$2" "$1" | sort -V | head -n 1)" = "$2" ]; }

failed=0
printf '%-18s %-9s %-9s %s\n' tool minimum found status
for entry in "${tools[@]}"; do
    IFS='|' read -r name command minimum <<< "$entry"
    if ! command -v "$name" > /dev/null; then
        printf '%-18s %-9s %-9s %s\n' "$name" "$minimum" - missing
        failed=1
        continue
    fi
    found=$($command 2>&1 | first_version)
    if at_least "$found" "$minimum"; then
        status=ok
    else
        status="too old"
        failed=1
    fi
    printf '%-18s %-9s %-9s %s\n' "$name" "$minimum" "$found" "$status"
done
exit $failed
