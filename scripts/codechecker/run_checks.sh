#!/usr/bin/env bash

# This script builds Libtropic in multiple configurations for CodeChecker evaluation.
#
# Majority of platforms are built using examples, as they are quicker to build,
# the model is built using tests, as they allow to change CAL, which has
# to be done for one target.

set -euo pipefail

LT_ROOT_DIR="."

if [ -z "$1" ]; then
    echo "Assuming Libtropic root directory is a current working directory."
    echo "To change the Libtropic root directory, pass it as the first argument:"
    echo "  $0 <path_to_libtropic>"
else
    LT_ROOT_DIR="${1%/}" # Remove last trailing slash (if any present)
    echo "Libtropic root directory set to: $LT_ROOT_DIR"
fi

echo "Checking dependencies..."
if ! command -v CodeChecker >/dev/null 2>&1; then
    echo "Missing CodeChecker! Install and try again."
    exit 1
fi
if ! command -v jq >/dev/null 2>&1; then
    echo "Missing jq! Install and try again."
    exit 1
fi
if ! command -v cmake >/dev/null 2>&1; then  
    echo "Missing cmake! Install and try again."  
    exit 1  
fi  
if ! command -v make >/dev/null 2>&1; then  
    echo "Missing make! Install and try again."  
    exit 1  
fi  

# Recreating directories
rm -fr "$LT_ROOT_DIR/.codechecker/"
mkdir -p "$LT_ROOT_DIR/.codechecker/compile_commands"
mkdir -p "$LT_ROOT_DIR/.codechecker/reports"
mkdir -p "$LT_ROOT_DIR/.codechecker/reports_html"

# Linux USB DevKit + MbedTLSv4
CodeChecker log -b "cd \"$LT_ROOT_DIR/examples/linux/usb_devkit/hello_world\" && rm -rf build && mkdir build && cd build && cmake .. && make -j" \
                -o "$LT_ROOT_DIR/.codechecker/compile_commands/usb_devkit_compile_commands.json"

# Linux SPI + MbedTLSv4
CodeChecker log -b "cd \"$LT_ROOT_DIR/examples/linux/spi/hello_world\" && rm -rf build && mkdir build && cd build && cmake .. && make -j" \
                -o "$LT_ROOT_DIR/.codechecker/compile_commands/linux_spi_compile_commands.json"

# Model + all CALs
CALS=("trezor_crypto" "mbedtls_v4" "openssl" "wolfcrypt")
for CURRENT_CAL in ${CALS[@]}; do
    CodeChecker log -b "cd \"$LT_ROOT_DIR/tests/functional/model\" && rm -rf build && mkdir build && cd build && cmake -DLT_CAL=$CURRENT_CAL .. && make -j" \
                    -o "$LT_ROOT_DIR/.codechecker/compile_commands/model_${CURRENT_CAL}_compile_commands.json"
done

# Merge compile_commands.json files
# Change temporarily to the directory, so we support
# also special symbols in the path specified by $LT_ROOT_DIR.
(cd "$LT_ROOT_DIR/.codechecker/compile_commands" && \
    jq -s 'add' ./*_compile_commands.json \
    > "../merged_compile_commands.json")

set +e
# Run analysis on merged compilation database
CodeChecker analyze "$LT_ROOT_DIR/.codechecker/merged_compile_commands.json" \
                    --config "$LT_ROOT_DIR/scripts/codechecker/codechecker_config.yml" \
                    --skip "$LT_ROOT_DIR/scripts/codechecker/codechecker.skip" \
                    -o "$LT_ROOT_DIR/.codechecker/reports"
set -euo pipefail

CodeChecker parse "$LT_ROOT_DIR/.codechecker/reports" \
                  -e html \
                  -o "$LT_ROOT_DIR/.codechecker/reports_html"