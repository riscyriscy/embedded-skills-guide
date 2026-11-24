#!/bin/bash
# Builds one image and runs it as three units: an erased one, two that the factory has written, and a
# record with one damaged byte. Usage: examples/provisioning/run.sh [work directory]
set -eu
root=$(git rev-parse --show-toplevel)
here=$root/examples/provisioning
work=${1:-$(mktemp -d)}
mkdir -p "$work"

# One image for every unit. The linker script keeps the last 4 KiB of the flash free. The run that
# examples/run.sh makes at the end finds no record and stops with status 1, which is not an error here.
cd "$root"
LD_SCRIPT=$here/link_prov.ld OUT=$work/unit.elf examples/run.sh m4 -O1 -Wall -Wextra -I"$here" \
    "$here/prov_demo.c" "$here/unit_record.c" > /dev/null || true
cd "$work"
sha256sum unit.elf | cut -c1-16,65-

# A factory key signs the certificates. Each unit makes its own key and gives the factory the public
# half.
openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:P-256 -out factory-ca.key 2> /dev/null
openssl req -new -x509 -key factory-ca.key -subj "/CN=sensorfw factory" -days 3650 -out factory-ca.pem
for n in 1 2; do
    serial=$(printf 'SF-%08d' "$n")
    openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:P-256 -out "unit-$n.key" 2> /dev/null
    openssl pkey -in "unit-$n.key" -pubout -out "unit-$n.pub" 2> /dev/null
    openssl req -new -key "unit-$n.key" -subj "/CN=$serial" |
        openssl x509 -req -CA factory-ca.pem -CAkey factory-ca.key -set_serial "$n" -days 3650 \
            -out "unit-$n.crt" 2> /dev/null
    openssl verify -CAfile factory-ca.pem "unit-$n.crt"
done

# The calibration comes from the test fixture: one offset and one gain for each unit.
make=("$here/provision.py" make --hw-rev 2 --image 0.2.0)
"${make[@]}" --serial SF-00000001 --mac 02:00:00:00:00:01 --offset -12 --gain 1002300 \
    --pubkey unit-1.pub --out unit-1.bin
"${make[@]}" --serial SF-00000002 --mac 02:00:00:00:00:02 --offset 7 --gain 997900 \
    --pubkey unit-2.pub --out unit-2.bin
cat factory.csv

# The same serial number twice is refused, and nothing is written.
"${make[@]}" --serial SF-00000001 --mac 02:00:00:00:00:03 --offset 0 --gain 1000000 \
    --pubkey unit-2.pub --out unit-3.bin || echo "exit status $?"

# A byte of the serial number is damaged after the factory wrote it. The tool reads the records back
# with code of its own, so a record that it accepts and the firmware rejects, or the reverse, shows up.
cp unit-1.bin damaged.bin
printf 'X' | dd of=damaged.bin bs=1 seek=12 conv=notrunc 2> /dev/null
head -c 4096 /dev/zero | tr '\0' '\377' > erased.bin
for record in unit-1 unit-2 damaged; do
    "$here/provision.py" show "$record.bin" || echo "exit status $?"
done

for record in erased unit-1 unit-2 damaged; do
    echo "--- $record"
    qemu-system-arm -machine mps2-an386 -nographic -semihosting-config enable=on,target=native \
        -device loader,file="$record.bin",addr=0x0007F000 -kernel unit.elf < /dev/null 2>&1 ||
        echo "exit status $?"
done
