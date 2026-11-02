#!/bin/bash
# Makes a release of the firmware from a tag and puts the files in build/release/<tag>.
# Usage: tools/release.sh keygen <directory>   creates a signing key (ECDSA P-256) and its public half
#        tools/release.sh make                 builds the tagged commit; SIGN_KEY names the private key
# The commit must carry a tag of the form vMAJOR.MINOR.PATCH and the tree must be clean. The release is
# built from nothing, tested, stamped with the tag, signed and listed in SHA256SUMS. tools/verify_release.sh
# checks the result. IMGTOOL replaces the way imgtool (the signing tool of MCUboot) is started.
set -eu
cd "$(git rev-parse --show-toplevel)"
imgtool=${IMGTOOL:-uvx --from imgtool==2.5.0 imgtool}

die() { echo "release: $*" >&2; exit 1; }

keygen() {
    local dir=${1:?usage: release.sh keygen <directory>}
    mkdir -p "$dir"
    [ ! -e "$dir/release-key.pem" ] || die "$dir/release-key.pem exists already, it is not replaced"
    ( umask 077 && $imgtool keygen -k "$dir/release-key.pem" -t ecdsa-p256 )
    openssl pkey -in "$dir/release-key.pem" -pubout -out "$dir/release-key.pub.pem"
    echo "private key: $dir/release-key.pem (keep it in the secret store of the pipeline)"
    echo "public key:  $dir/release-key.pub.pem (goes into the bootloader and the verify step)"
}

make_release() {
    : "${SIGN_KEY:?SIGN_KEY is not set: a release is always signed}"
    [ -r "$SIGN_KEY" ] || die "cannot read the key $SIGN_KEY"

    # The tag is the version. A commit after the tag, a dirty tree or a clone without tags stop the release here.
    local describe tag version
    describe=$(git describe --tags --always --dirty)
    tag=$(git describe --tags --exact-match 2> /dev/null) || die "HEAD is not on a tag: git describe says $describe"
    [[ $tag =~ ^v([0-9]+\.[0-9]+\.[0-9]+)$ ]] || die "the tag $tag is not of the form vMAJOR.MINOR.PATCH"
    version=${BASH_REMATCH[1]}
    local cmake_version
    cmake_version=$(sed -n 's/^project(sensorfw VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
    [ "$cmake_version" = "$version" ] || die "CMakeLists.txt says version $cmake_version but the tag is $tag"
    [ -z "$(git status --porcelain)" ] || die "the tree has uncommitted changes: git describe says $describe"

    # The build starts from nothing. -g adds debug information and changes no instruction.
    local build=build/m4-release elf=build/m4-release/app/sensor_demo
    rm -rf "$build"
    cmake --preset m4-release -DCMAKE_C_FLAGS_RELEASE="-Os -g -DNDEBUG" > /dev/null
    cmake --build --preset m4-release > /dev/null
    ctest --preset m4-release > /dev/null || die "the tests failed on the target"
    # The binary has to say what the tag says.
    strings "$elf" | grep -qx "$version ($describe)" || die "the version string in the image is not $version ($describe)"

    local out=build/release/$tag name=sensorfw-$tag
    rm -rf "$out" && mkdir -p "$out"
    cp "$elf" "$out/$name.debug.elf"
    cp "$elf.map" "$out/$name.map"
    arm-none-eabi-objcopy -O binary "$elf" "$out/$name.bin"
    $imgtool sign --key "$SIGN_KEY" --header-size 0x200 --align 4 --slot-size 0x40000 --pad-header \
        --version "$version" "$out/$name.bin" "$out/$name.signed.bin"

    # Release notes: the subjects of the commits since the previous tag.
    local previous range
    previous=$(git describe --tags --abbrev=0 "$tag^" 2> /dev/null || true)
    range=${previous:+$previous..}$tag
    {
        echo "# sensorfw $tag"
        echo
        if [ -n "$previous" ]; then echo "Changes since $previous:"; else echo "First release."; fi
        echo
        git log --no-merges --pretty='- %s (%h)' "$range"
    } > "$out/RELEASE-NOTES.md"

    # The manifest says what the files are and how they were made. The time is the commit time, not the build time.
    local size
    size=$(arm-none-eabi-size "$elf" | awk 'NR == 2 { printf "\"flash\": %d, \"ram\": %d", $1 + $2, $2 + $3 }')
    printf '{"name": "sensorfw", "version": "%s", "tag": "%s", "commit": "%s", "commit_time": "%s", "compiler": "%s", %s}\n' \
        "$version" "$tag" "$(git rev-parse HEAD)" "$(git log -1 --format=%cI)" \
        "$(arm-none-eabi-gcc --version | head -n 1)" "$size" > "$out/manifest.json"

    # The list of hashes is signed. A download is checked against the list, the list against the signature.
    (cd "$out" && sha256sum -- * > SHA256SUMS)
    openssl dgst -sha256 -sign "$SIGN_KEY" -out "$out/SHA256SUMS.sig" "$out/SHA256SUMS"
    local files=("$out"/*)
    echo "release $tag: ${#files[@]} files in $out"
}

case ${1:-make} in
    keygen) shift; keygen "$@" ;;
    make) make_release ;;
    *) die "usage: release.sh keygen <directory> | make" ;;
esac
