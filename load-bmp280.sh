#!/bin/bash
set -e

modprobe regmap-i2c
modprobe industrialio
dtoverlay ./bmp280-overlay.dtbo
echo "Loaded device tree overlay"

rmmod bmp280_pi 2>/dev/null || true
insmod bmp280_pi.ko
echo "Loaded driver bmp_280_pi.ko"
