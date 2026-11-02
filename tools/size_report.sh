#!/bin/bash
# Reports flash and RAM use of an ELF file against the limits of the part.
# Usage: tools/size_report.sh <elf> [flash_bytes] [ram_bytes]
# Exit status 1 when a limit is exceeded, 2 on a usage error.
set -euo pipefail

elf=${1:?usage: size_report.sh <elf> [flash_bytes] [ram_bytes]}
flash_limit=${2:-262144}
ram_limit=${3:-65536}

[ -f "$elf" ] || { echo "size_report.sh: no such file: $elf" >&2; exit 2; }

# Berkeley format: text data bss dec hex filename. Flash holds text and the initial values of data.
read -r flash ram < <(arm-none-eabi-size "$elf" | awk 'NR == 2 { print $1 + $2, $2 + $3 }')

status=0
report() {
    local name=$1 used=$2 limit=$3 verdict=ok
    if [ "$used" -gt "$limit" ]; then verdict=OVER; status=1; fi
    awk -v n="$name" -v u="$used" -v l="$limit" -v v="$verdict" \
        'BEGIN { printf "%-6s %7d of %7d bytes %5.1f%%  %s\n", n, u, l, 100 * u / l, v }'
}
report flash "$flash" "$flash_limit"
report ram "$ram" "$ram_limit"
exit $status
