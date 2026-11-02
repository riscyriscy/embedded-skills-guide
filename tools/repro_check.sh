#!/bin/sh
# Builds the committed source twice, in two directories with different names and depths,
# and compares the SHA-256 of the images. Exit status 0 means the builds are identical.
set -eu
src=$(git rev-parse --show-toplevel)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

build_and_hash() {
    git clone -q "$src" "$1"
    (cd "$1" && cmake --workflow --preset ci-m4 > /dev/null)
    (cd "$1/build/m4-release" && sha256sum app/sensor_demo app/sensor_demo.bin | cut -c1-16,65-)
}

first=$(build_and_hash "$work/a/src")
second=$(build_and_hash "$work/deeper/path/b/src")
printf 'a/src\n%s\ndeeper/path/b/src\n%s\n' "$first" "$second"

if [ "$first" = "$second" ]; then
    echo "reproducible: both builds are identical"
else
    echo "NOT reproducible: the builds differ"
    exit 1
fi
