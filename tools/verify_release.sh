#!/bin/bash
# Checks a release directory the way a receiver does: the hashes, the signature of the list of hashes,
# the signature of the image and the version in the manifest, the file name and the binary.
# Usage: tools/verify_release.sh <release directory> <public key>
# Exit status 0 when every check passes. Every check runs, so one run shows every problem.
set -u
dir=${1:?usage: verify_release.sh <release directory> <public key>}
pub=${2:?usage: verify_release.sh <release directory> <public key>}
imgtool=${IMGTOOL:-uvx --from imgtool==2.5.0 imgtool}
failed=0

check() {
    local what=$1
    shift
    if "$@" > /dev/null 2>&1; then printf 'ok      %s\n' "$what"; else printf 'FAILED  %s\n' "$what"; failed=1; fi
}

cd "$dir" || exit 2
tag=$(basename "$PWD")
name=sensorfw-$tag
check "SHA256SUMS lists the files and the hashes match" sha256sum --check --strict SHA256SUMS
check "SHA256SUMS.sig is a signature of SHA256SUMS" openssl dgst -sha256 -verify "$pub" -signature SHA256SUMS.sig SHA256SUMS
# shellcheck disable=SC2086
check "$name.signed.bin is signed with the release key" $imgtool verify -k "$pub" "$name.signed.bin"
check "manifest.json carries the version ${tag#v}" grep -q "\"version\": \"${tag#v}\"" manifest.json
# shellcheck disable=SC2317
has_string() { tr '\0' '\n' < "$1" | grep -qx "$2"; }
check "the binary carries the string ${tag#v} ($tag)" has_string "$name.bin" "${tag#v} ($tag)"
exit $failed
