#!/bin/bash
# Builds one example for the Cortex-M4 of QEMU (the board mps2/an386) with the Arm GNU Toolchain that is
# installed on the machine, not the Zephyr SDK. Usage: examples/zephyr/build.sh <app> [cmake options]
# Set ZEPHYR_WS to the workspace directory that setup.sh created.
set -eu
app=${1:?usage: build.sh <app> [cmake options]}
shift
here=$(cd "$(dirname "$0")" && pwd)
ws=${ZEPHYR_WS:-$HOME/zephyr-ws/zephyrproject}
export ZEPHYR_BASE=$ws/zephyr ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb GNUARMEMB_TOOLCHAIN_PATH=${GNUARMEMB_TOOLCHAIN_PATH:-/usr}
build=${BUILD_DIR:-$here/../../build/zephyr-$app}
cd "$ZEPHYR_BASE"
west build -p auto -b mps2/an386 "$here/$app" -d "$build" ${1:+--} "$@"
