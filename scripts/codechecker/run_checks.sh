#!/usr/bin/env bash

# This script builds Libtropic in multiple configurations for CodeChecker evaluation.
#
# Majority of platforms are built using examples, as they are quicker to build,
# the model is built using tests, as they allow to change CAL, which has
# to be done for one target.

set -e

LT_ROOT_DIR="./"

if [ -z $1 ]; then
    echo "Assuming Libtropic root directory is a current working directory."
    echo "To change the Libtropic root directory, pass it as the first argument:"
    echo "  $0 <path_to_libtropic>"
else
    LT_ROOT_DIR="$1"
    echo "Libtropic root directory set to: $LT_ROOT_DIR"
fi

# Linux USB DevKit + MbedTLSv4
CodeChecker log -b "cd \"$LT_ROOT_DIR/examples/linux/usb_devkit/hello_world\" && rm -rf build && mkdir build && cd build && cmake .. && make -j" \
                -o "$LT_ROOT_DIR/.codechecker/reports/usb_devkit"

# Linux SPI + MbedTLSv4
CodeChecker log -b "cd \"$LT_ROOT_DIR/examples/linux/spi/hello_world\" && rm -rf build && mkdir build && cd build && cmake .. && make -j" \
                -o "$LT_ROOT_DIR/.codechecker/reports/linux_spi"

# Model + all CALs
CALS=("trezor_crypto" "mbedtls_v4" "openssl" "wolfcrypt")
for CURRENT_CAL in ${CALS[@]}; do
    CodeChecker log -b "cd \"$LT_ROOT_DIR/tests/functional/model\" && rm -rf build && mkdir build && cd build && cmake -DLT_CAL=$CURRENT_CAL .. && make -j" \
                    -o "$LT_ROOT_DIR/.codechecker/reports/model"
done

# Merge compile_commands.json files
jq -s 'add' "$LT_ROOT_DIR/.codechecker/reports/model" \
            "$LT_ROOT_DIR/.codechecker/reports/usb_devkit" \
            "$LT_ROOT_DIR/.codechecker/reports/linux_spi" \
            > "$LT_ROOT_DIR/.codechecker/compile_commands.json"

set +e
# Run analysis on merged compilation database
CodeChecker analyze "$LT_ROOT_DIR/.codechecker/compile_commands.json" \
                    --config "$LT_ROOT_DIR/scripts/codechecker/codechecker_config.yml" \
                    --skip "$LT_ROOT_DIR/scripts/codechecker/codechecker.skip" \
                    -o "$LT_ROOT_DIR/.codechecker/reports/merged"
set -e

CodeChecker parse "$LT_ROOT_DIR/.codechecker/reports/merged" \
                  -e html \
                  -o "$LT_ROOT_DIR/.codechecker/reports_html"