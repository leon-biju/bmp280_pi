#!/bin/bash


# Creates a fake IIO sysfs tree and writes synthetic sensor values.
# Usage: ./mock_iio.sh [directory] [interval_s]
#   Then run the daemon with: ./bmp280d -d <directory>

MOCK_DIR="${1:-/tmp/fake_iio}"
INTERVAL="${2:-2}"

mkdir -p "$MOCK_DIR"

echo "mock iio device at $MOCK_DIR (updating every ${INTERVAL}s)"
echo "run daemon with: ./bmp280d -d $MOCK_DIR"

while true; do
    # about 25°C +/- 0.5°C (millidegrees), abt 101.325 kPa +/- 0.05 kPa
    TEMP=$(( 25000 + (RANDOM % 1001) - 500 ))
    PRES=$(awk "BEGIN {printf \"%.6f\", 101.325 + ($RANDOM % 101 - 50) / 1000}")

    echo "$TEMP" > "$MOCK_DIR/in_temp_input"
    echo "$PRES" > "$MOCK_DIR/in_pressure_input"

    sleep "$INTERVAL"
done
