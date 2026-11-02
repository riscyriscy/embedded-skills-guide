#!/bin/bash
# Makes a RAUC bundle that carries a root file system image, with a test CA and a test signing certificate.
# Usage: make_bundle.sh <rootfs image> <version> <work directory>
# The work directory gets ca.crt (the keyring of the device), release.crt and release.key (the signer)
# and sensorfw-<version>.raucb. A product keeps the CA key offline and the signing key in a secret store.
set -eu
rootfs=$(realpath "${1:?usage: make_bundle.sh <rootfs image> <version> <work directory>}")
version=${2:?usage: make_bundle.sh <rootfs image> <version> <work directory>}
work=${3:?usage: make_bundle.sh <rootfs image> <version> <work directory>}
mkdir -p "$work/content"
cd "$work"

if [ ! -e ca.crt ]; then
    openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -nodes -keyout ca.key -out ca.crt \
        -days 3650 -subj "/O=Example/CN=sensorfw CA" \
        -addext "basicConstraints=critical,CA:TRUE" -addext "keyUsage=critical,keyCertSign,cRLSign" 2> /dev/null
    openssl req -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -nodes -keyout release.key -out release.csr \
        -subj "/O=Example/CN=sensorfw release" 2> /dev/null
    # No extended key usage: RAUC checks the S/MIME signing purpose, which a "code signing" certificate fails.
    printf 'basicConstraints=CA:FALSE\nkeyUsage=digitalSignature\n' > release.ext
    openssl x509 -req -in release.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out release.crt -days 365 \
        -extfile release.ext 2> /dev/null
fi

cp "$rootfs" content/rootfs.img
cat > content/manifest.raucm << EOM
[update]
compatible=sensorfw-qemu-virt
version=$version

[bundle]
format=verity

[image.rootfs]
filename=rootfs.img
EOM
rm -f "sensorfw-$version.raucb"
rauc bundle --cert release.crt --key release.key content "sensorfw-$version.raucb"
