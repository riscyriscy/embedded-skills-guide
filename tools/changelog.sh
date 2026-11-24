#!/bin/bash
# Writes a changelog section from the commit messages since a tag, and the version
# that the changes call for. A subject starts with a type (feat, fix, perf, docs,
# test, build, refactor or chore) and ": text". A "!" after the type, or a line
# "BREAKING CHANGE:" in a message, marks an incompatible change.
# The section goes to standard output, a one line summary to standard error.
# Usage: tools/changelog.sh <previous tag> [revision]    (default revision: HEAD)
set -euo pipefail

prev=${1:?usage: changelog.sh <previous tag> [revision]}
rev=${2:-HEAD}
IFS=. read -r major minor patch <<< "${prev#v}"

declare -A section titles=([feat]="Features" [fix]="Fixes" [perf]="Performance"
    [refactor]="Internal changes" [docs]="Documentation" [test]="Tests"
    [build]="Build" [chore]="Other")
order=(feat fix perf refactor docs test build chore)
features=0 fixes=0 breaking=0

if git log --format=%B "$prev..$rev" | grep -q '^BREAKING CHANGE:'; then
    breaking=1
fi
while IFS= read -r subject; do
    type=${subject%%:*}
    case $type in *'!') breaking=1; type=${type%!} ;; esac
    text=${subject#*: }
    case $type in feat) features=$((features + 1)) ;; fix) fixes=$((fixes + 1)) ;; esac
    [ -n "${titles[$type]:-}" ] || type=chore
    section[$type]+="- $text"$'\n'
done < <(git log --format=%s "$prev..$rev")

if [ "$breaking" = 1 ]; then
    next="$((major + 1)).0.0"
elif [ "$features" -gt 0 ]; then
    next="$major.$((minor + 1)).0"
else
    next="$major.$minor.$((patch + 1))"
fi

echo "## $next"
echo
for t in "${order[@]}"; do
    [ -n "${section[$t]:-}" ] || continue
    echo "### ${titles[$t]}"
    printf '%s\n' "${section[$t]}"
done
verdict=no
[ "$breaking" = 1 ] && verdict=yes
echo "Since $prev: $features feature(s), $fixes fix(es)," \
     "incompatible changes: $verdict." >&2
