#!/bin/sh
# Builds the committed source twice, in two directories with different names and depths,
# and compares the SHA-256 of the images. Exit status 0 means the builds are identical.
set -eu
src=$(git rev-parse --show-toplevel)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
sums=""
for dir in "$work/a/src" "$work/deeper/path/b/src"; do
    git clone -q "$src" "$dir"
    (cd "$dir" && cmake --workflow --preset ci-m4 > /dev/null)
    sums="$sums$(cd "$dir/build/m4-release" && sha256sum app/sensor_demo app/sensor_demo.bin | cut -c1-16,65-)
"
    printf '%s\n' "$sums" | tail -n 3 | sed "s#^#$dir  #" | sed "s#$work/##"
done
first=$(printf '%s' "$sums" | sed -n '1,2p')
second=$(printf '%s' "$sums" | sed -n '3,4p')
if [ "$first" = "$second" ]; then
    echo "reproducible: both builds are identical"
else
    echo "NOT reproducible: the builds differ"
    exit 1
fi
