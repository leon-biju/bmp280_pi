# bmp280_pi

A custom Linux IIO kernel driver for the BMP280 sensor on the Raspberry Pi,
plus a userspace daemon that polls the sensor and serves the readings.

Both components can be built two ways: **natively** on the Pi (simple, but might be long depending on your hardware), or **cross-compiled** from another machine (faster, but the driver needs a synced kernel source tree). 
The same `CROSS_COMPILE` switch selects the mode for each Makefile: leave it unset to build natively, set `CROSS_COMPILE=aarch64-linux-gnu-` to cross-compile.

> **Warning for kernel updates.** The driver is tied to the exact kernel it was built against. Whenever the Pi's kernel is upgraded, the old module stops loading into the new kernel.
> So after a kernal update, you must rebuild and reinstall it. This applies to both build modes.
> Native build needs the new `linux-headers-$(uname -r)` (This should automatically be updated using any good package manager).
> Cross-compiled build needs the `rpi-linux` source re-synced to the new kernel (see below). 
> The daemon is userspace so is unaffected in both cases.

## Native Building (Easy)

Building directly on the Pi. Install the build tools and the
headers for the running kernel:

```bash
sudo apt install build-essential device-tree-compiler linux-headers-$(uname -r)
```

Clone this repo on the Pi. With `CROSS_COMPILE` unset (the default value), each
Makefile builds the driver against the running kernel's headers at
`/lib/modules/$(uname -r)/build`.

Build the driver (module lands in `driver/build/bmp280_pi.ko`):

```bash
cd driver
make
```

Compile the device tree overlay:

```bash
dtc -@ -I dts -O dtb -o bmp280-overlay.dtbo bmp280-overlay.dts
```

Build the daemon (binary lands in `daemon/build/bmp280d`):

```bash
cd daemon
make
```

Everything is now built in place on the Pi.
See [Driver usage](#driver-usage) to load the driver and [Daemon usage](#daemon-usage) to run the daemon service.

## Cross-compiling (A bit more advanced)
If you prefer not to compile on your pi hardware, whether that be for time constraints or whatever, you can compile directly from a more powerful machine.

Install the following on your build machine:

- `aarch64-linux-gnu-gcc`
- `make`
- `dtc` (device-tree compiler)

### Kernel source

Clone the RPi kernel fork alongside the project and prepare it for out-of-tree module builds. The source checkout must match the kernel the Pi is running (`uname -r`, e.g. `6.18.33+rpt-rpi-v8`): use the matching `rpi-6.XX.y` branch and check out the last commit before the SUBLEVEL is bumped past the Pi's version.

```bash
git clone --depth 1 --branch <branch> https://github.com/raspberrypi/linux.git rpi-linux

# Copy the running kernel's .config from the Pi into rpi-linux/

scp pi@<pi-ip>:/boot/config-$(ssh pi@<pi-ip> uname -r) rpi-linux/.config

# Copy Module.symvers from the Pi as well
scp pi@<pi-ip>:/lib/modules/$(ssh pi@<pi-ip> uname -r)/build/Module.symvers rpi-linux/
```

The Debian packaging overrides the localversion at build time, so the copied
`.config` says `CONFIG_LOCALVERSION="-v8"` even though the kernel release is
`...+rpt-rpi-v8`. Set it to the real suffix so the module's vermagic matches,
and pass an empty `LOCALVERSION=` so git doesn't append a stray `+`:

```bash
sed -i 's|^CONFIG_LOCALVERSION=.*|CONFIG_LOCALVERSION="+rpt-rpi-v8"|' rpi-linux/.config

make -C rpi-linux ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LOCALVERSION= olddefconfig
make -C rpi-linux ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LOCALVERSION= modules_prepare

# Verify it matches uname -r on the Pi exactly:
cat rpi-linux/include/generated/utsrelease.h
```

**After a kernel update on the Pi**, redo all of the above (fetch/checkout the
matching commit, re-copy `.config` and `Module.symvers`, re-run `olddefconfig`
and `modules_prepare`), then rebuild and redeploy the module.

### Build the driver

Setting `CROSS_COMPILE` switches the driver Makefile to cross mode: it builds
against the `rpi-linux` tree prepared above with `ARCH=arm64`.

```bash
cd driver
make CROSS_COMPILE=aarch64-linux-gnu-
```

The module ends up in `driver/build/bmp280_pi.ko`.

### Build the daemon

The daemon uses the same switch, routed through `CC`, so it needs no kernel
tree:

```bash
cd daemon
make CROSS_COMPILE=aarch64-linux-gnu-
```

The binary lands in `daemon/build/bmp280d`.

### Build the device tree overlay

Compile the device tree source into a `.dtbo` overlay:

```bash
dtc -@ -I dts -O dtb -o bmp280-overlay.dtbo bmp280-overlay.dts
```

### Deploy

Copy the artifacts and helper scripts to the Pi:

```bash
scp driver/build/bmp280_pi.ko bmp280-overlay.dtbo install-bmp280.sh load-bmp280.sh pi@<pi-ip>:~/
scp daemon/build/bmp280d pi@<pi-ip>:~/
```

Then load the driver and run the daemon as described below.

## Driver usage

There are two ways to load the driver on the Pi. Both expect `bmp280-overlay.dtbo` and `bmp280_pi.ko` to be present ( doesn't matter if built in place by a native build or copied over from a cross-compiled build).

**Persistent (recommended).** Install once so everything loads automatically on every boot:

```bash
sudo ./install-bmp280.sh
sudo reboot
```

This installs the overlay into the boot overlays directory and adds `dtoverlay=bmp280-overlay` to `config.txt`, and installs the module into `/lib/modules/$(uname -r)/extra/` followed by `depmod`.


**Temporary (dev/testing).** Load everything for the current session only, without touching the boot config:
Note: This will mean it won't remain loaded after a reboot.
```bash
sudo ./load-bmp280.sh
```

This loads the `regmap-i2c` and `industrialio` dependencies, applies the overlay
at runtime, and inserts the module.



Either way, once the driver is loaded it registers an IIO device, usually at `/sys/bus/iio/devices/iio:device0/` or something similar. Two read-only attributes are exposed:

```bash
# Temperature in millidegrees Celsius
cat /sys/bus/iio/devices/iio:device0/in_temp_input

# Pressure in kPa
cat /sys/bus/iio/devices/iio:device0/in_pressure_input
```

## Daemon usage

The daemon (`daemon/`) polls the IIO device the driver exposes, computes rolling
statistics (min/max/mean for temperature and pressure), and serves the data over
a small HTTP API. A background thread does the polling and publishes a
mutex-protected snapshot; the main thread runs the HTTP server.

It defaults to polling `/sys/bus/iio/devices/iio:device0` every 2 seconds and
serving on port 8080. Override with `-d <path>`, `-i <seconds>`, and `-p <port>`.
For local development without hardware, `mock_iio.sh` populates a fake sysfs tree
you can point the daemon at.

### HTTP API

All endpoints are read-only `GET`s returning JSON. Pressure is reported in
hectopascals (hPa).

```bash
# Latest reading
curl http://<pi-ip>:8080/api/v1/current
# {"temperature_c":22.45,"pressure_hpa":1013.25,"timestamp":"2026-06-12T14:30:00Z"}

# Rolling statistics over the recent window
curl http://<pi-ip>:8080/api/v1/average
# {"temperature_c":{"mean":..,"min":..,"max":..},"pressure_hpa":{...},"samples":12}

# Tripped thresholds (alert logic is the next milestone)
curl http://<pi-ip>:8080/api/v1/alerts
# {"alerts":[]}
```

### Running as a service

The daemon is designed to be run under systemd. Install the binary and unit, then
enable it:

```bash
sudo make install
sudo systemctl daemon-reload
sudo systemctl enable --now bmp280d
```

It logs to the journal (`journalctl -u bmp280d -f`), restarts on failure (which
also covers the case where the driver isn't loaded yet), and runs locked down
with `DynamicUser` and a read-only filesystem view. `sudo make uninstall`
removes it.

## License

This project uses different licenses for its two components:

- **`driver/`**: GNU General Public License v2.0 only ([`driver/LICENSE`](driver/LICENSE)). The kernel driver uses GPL-only kernel symbols like IIO, regmap, etc. which requires GPLv2 compatibility.
- **`daemon/`**: MIT License ([`daemon/LICENSE`](daemon/LICENSE)). The userspace daemon communicates with the driver only through sysfs, so it is not a derived work of the kernel and can use a permissive license.
