#!/bin/bash
# Looks for private keys, cloud access keys and assigned secrets in the files of a
# directory. Prints file:line for every finding, never the secret itself.
# Exit status 1 when there is a finding.
# Usage: tools/secret_scan.sh [directory]     (default: the current directory)
set -uo pipefail

dir=${1:-.}
status=0
names='password|passwd|secret|api_?key|token'
scan() {
    local label=$1 pattern=$2
    # -I skips binary files and -n numbers the lines. cut keeps file and line, so
    # the secret is never printed.
    grep -rIEin --exclude-dir=.git --exclude-dir=build --exclude='*.lock' \
        --exclude=secret_scan.sh -e "$pattern" "$dir" 2>/dev/null \
        | cut -d: -f1,2 | sed "s|\$| $label|" && status=1
}
scan "private key block"   '-----BEGIN ([A-Z]+ )?PRIVATE KEY-----'
scan "cloud access key id" 'AKIA[0-9A-Z]{16}'
scan "assigned secret" \
    "($names)[A-Za-z_]*[[:space:]]*[:=][[:space:]]*\"[^\"\$<{][^\"]{7,}\""
scan "secret in a #define" \
    "#define[[:space:]]+[A-Za-z_]*($names)[A-Za-z_]*[[:space:]]+\"[^\"]{7,}\""
[ "$status" = 0 ] && echo "no findings in $dir"
exit "$status"
