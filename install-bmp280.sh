#!/bin/bash
# Permanently installs the BMP280 driver + overlay on the Pi so it loads automatically on boot. 
# Run this on the Pi as root, from the directory
# holding bmp280_pi.ko and bmp280-overlay.dtbo.

set -e

KO=bmp280_pi.ko
DTBO=bmp280-overlay.dtbo
OVERLAY_NAME=bmp280-overlay

if [[ $EUID -ne 0 ]]; then
	echo "Must run the installation as root, because you will need to modify the kernel a little bit" >&2
	exit 1
fi

[[ -f $KO ]]   || { echo "missing $KO in current directory" >&2; exit 1; }
[[ -f $DTBO ]] || { echo "missing $DTBO in current directory" >&2; exit 1; }

# Boot config moved to /boot/firmware on recent Raspberry Pi OS.
if [[ -f /boot/firmware/config.txt ]]; then
	BOOT=/boot/firmware
elif [[ -f /boot/config.txt ]]; then
	BOOT=/boot
else
	echo "could not find config.txt under /boot/firmware or /boot" >&2
	exit 1
fi

KREL=$(uname -r)
MODDIR=/lib/modules/$KREL/extra

# 1. Install the module and refresh the modalias map.
install -D -m 0644 "$KO" "$MODDIR/$KO"
depmod -a
echo "installed $KO to $MODDIR and ran depmod"

# 2. Install the overlay.
install -D -m 0644 "$DTBO" "$BOOT/overlays/$DTBO"
echo "installed $DTBO to $BOOT/overlays"

# 3. Enable the overlay at boot (idempotent).
if grep -qE "^\s*dtoverlay=$OVERLAY_NAME\b" "$BOOT/config.txt"; then
	echo "dtoverlay=$OVERLAY_NAME already present in $BOOT/config.txt"
else
	printf '\ndtoverlay=%s\n' "$OVERLAY_NAME" >> "$BOOT/config.txt"
	echo "added dtoverlay=$OVERLAY_NAME to $BOOT/config.txt"
fi

echo
echo "Done. Reboot to apply: the overlay creates the I2C device and the kernel"
echo "autoloads bmp280_pi (and its IIO/regmap deps) via the modalias."
