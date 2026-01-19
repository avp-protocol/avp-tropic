#!/usr/bin/env bash

# --- 1. ARGUMENT VALIDATION ---
if [[ -z "${1:-}" || -z "${2:-}" || -z "${3:-}" || -z "${4:-}" || -z "${5:-}" ]]; then
    echo "libtropic ESP32 script for flashing and getting log from tests"
    echo "usage: ./run_test.sh PATH_TO_BUILD_DIR PATH_TO_SERIAL_FD TARGET BAUD_FLASH BAUD_SERIAL_MONITOR"
    exit 1
fi

BUILD_DIR="$1"
SERIAL_FD="$2"
TARGET="$3"
BAUD_FLASH="$4"
BAUD_SERIAL_MONITOR="$5"

SENTINEL_OK="TEST FINISHED"
SENTINEL_FAIL_1="ASSERT FAIL"
SENTINEL_FAIL_2="WARNING"
SENTINEL_FAIL_3="ERROR"

MONITOR_PID=""
LOGFILE="$(mktemp)"

# Derive ELF name from last directory in BUILD_DIR (basename) + .elf
ELF_NAME="$(basename "${BUILD_DIR}")".elf
ELF_PATH="${BUILD_DIR%/}/${ELF_NAME}"

set -euo pipefail

cleanup() {
    # Stop monitor if still running
    if [[ -n "${MONITOR_PID}" ]]; then
        kill "${MONITOR_PID}" 2>/dev/null || true
        wait "${MONITOR_PID}" 2>/dev/null || true
        MONITOR_PID=""
    fi
}
trap cleanup EXIT

# --- 2. FLASH DEVICE ---
pushd "${BUILD_DIR}" >/dev/null
echo "Flashing device at ${SERIAL_FD}..."
esptool.py --chip $TARGET --port $SERIAL_FD --baud $BAUD_FLASH --after no_reset write_flash @flash_project_args
popd >/dev/null

# --- 3. MONITOR (PTY wrapper, no timeout) ---
echo "Starting monitor on ${SERIAL_FD}..."

# Python PTY wrapper for esp-idf-monitor (accept optional ELF)
# Is necessary because esp-idf-monitor does not support PTY directly (it detects stdin is not a TTY)
cat > /tmp/esp32_monitor_wrapper.py << 'EOF'
import pty, sys
port = sys.argv[1]
baud = sys.argv[2]
elf = sys.argv[3]
target = sys.argv[4]
args = [sys.executable, "-m", "esp_idf_monitor", "--port", port, "--baud", baud, "--target", target, elf]
pty.spawn(args)
EOF

# Ensure logfile exists before tail starts
: > "${LOGFILE}"

# Start monitor writing to logfile
python3 /tmp/esp32_monitor_wrapper.py "${SERIAL_FD}" "${BAUD_SERIAL_MONITOR}" "${ELF_PATH}" "${TARGET}" >"${LOGFILE}" 2>&1 &
MONITOR_PID=$!

# Follow log until sentinel; no pipeline => no subshell
TEST_FAILED=0
while IFS= read -r line; do
    printf '%s\n' "$line"

    if [[ "$line" == *"$SENTINEL_FAIL_1"* ]] \
    || [[ "$line" == *"$SENTINEL_FAIL_2"* ]] \
    || [[ "$line" == *"$SENTINEL_FAIL_3"* ]]; then
        TEST_FAILED=1
    elif [[ "$line" == *"$SENTINEL_OK"* ]]; then
        break
    fi
done < <(tail -n0 -F --pid="${MONITOR_PID}" "${LOGFILE}")

# --- 4. CLEANUP ---
cleanup
rm -f "${LOGFILE}"
exit $TEST_FAILED