#!/bin/bash
# The path of the book in one script: a fresh clone, the stages of the pipeline, the documentation, a release
# from a tag, the check of that release and its installation on a simulated device with two slots.
# Usage: examples/capstone.sh <work directory> <new version>        for example: examples/capstone.sh /tmp/capstone 0.2.0
# The script stops at the first step that fails. The old image of the device is the build before the release.
set -eu
work=$(realpath -m "${1:?usage: capstone.sh <work directory> <new version>}")
version=${2:?usage: capstone.sh <work directory> <new version>}
src=$(git rev-parse --show-toplevel)
sim=$src/examples/update/ab_sim.py
step() { printf '== %s\n' "$*"; }

rm -rf "$work" && mkdir -p "$work"
step "clone"
git clone -q "$src" "$work/sensorfw"
cd "$work/sensorfw"
git describe --tags --always

step "pipeline: env static host asan coverage m4 docs meta"
tools/ci.sh env static host asan coverage m4 docs meta
cp build/m4-release/app/sensor_demo.bin "$work/old.bin"
old=$(sed -n 's/.*"version": "\([^"]*\)".*/\1/p' build/reports/meta.json)

step "release $version: raise the version, commit, tag"
sed -i "s/^project(sensorfw VERSION [0-9.]*/project(sensorfw VERSION $version/" CMakeLists.txt
git -c user.name=capstone -c user.email=capstone@example.com commit -qam "Release $version"
git -c user.name=capstone -c user.email=capstone@example.com tag "v$version"
tools/release.sh keygen "$work/keys" > /dev/null
SIGN_KEY=$work/keys/release-key.pem tools/release.sh make

step "verify the release"
tools/verify_release.sh "build/release/v$version" "$work/keys/release-key.pub.pem"

step "install on the device"
python3 "$sim" --dir "$work/device" init "$work/old.bin" "$old" > /dev/null
python3 "$sim" --dir "$work/device" install "build/release/v$version/sensorfw-v$version.signed.bin" "$version"
python3 "$sim" --dir "$work/device" boot
python3 "$sim" --dir "$work/device" status
