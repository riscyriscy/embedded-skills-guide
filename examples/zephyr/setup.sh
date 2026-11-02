#!/bin/bash
# Creates a Zephyr workspace with only the modules that the examples need, and installs the Python
# packages of the build. Usage: examples/zephyr/setup.sh [workspace directory]
# Needs: git, python3 with pip, CMake, Ninja, dtc, gperf and arm-none-eabi-gcc.
set -eu
ws=${1:-$HOME/zephyr-ws}
version=v4.5.0
pip install --user --break-system-packages west
mkdir -p "$ws" && cd "$ws"
west init -m https://github.com/zephyrproject-rtos/zephyr --mr "$version" zephyrproject
cd zephyrproject
west config manifest.group-filter -- -optional
# Not all of the modules: the Cortex-M3 and Cortex-M4 examples need the CMSIS headers and the STM32
# HAL that the manifest lists for the Arm boards.
west update --narrow -o=--depth=1 cmsis cmsis_6 hal_stm32
pip install --user --break-system-packages -r zephyr/scripts/requirements-base.txt
echo "workspace: $PWD"
