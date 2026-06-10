# bmp280_pi

A custom Linux IIO kernel driver for the BMP280 sensor on the Raspberry Pi.

Note: A daemon will be implemented soon 

There are two ways to build the driver: directly on the Pi (simple), or
cross-compiling from another machine (faster, but does requires syncing the kernel
source)

## Building on the Pi

The easy path. Install the build tools and the headers for the running kernel:

```bash
sudo apt install build-essential device-tree-compiler linux-headers-$(uname -r)
```

Clone this repo on the Pi, then build the driver against the installed headers
(overriding the Makefile defaults, which are set up for cross-compiling):

```bash
cd driver
make KDIR=/lib/modules/$(uname -r)/build CROSS_COMPILE=
```

Compile the device tree overlay:

```bash
dtc -@ -I dts -O dtb -o bmp280-overlay.dtbo bmp280-overlay.dts
```

Then load everything with the loader script (expects `bmp280-overlay.dtbo` and
`bmp280_pi.ko` in the current directory):

```bash
sudo ./load-bmp280.sh
```

## Cross-compiling

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

```bash
cd driver
make
```

The module ends up in `driver/build/bmp280_pi.ko`.

### Build the device tree overlay

Compile the device tree source into a `.dtbo` overlay:

```bash
dtc -@ -I dts -O dtb -o bmp280-overlay.dtbo bmp280-overlay.dts
```

### Deploy

Copy `bmp280_pi.ko` and `bmp280-overlay.dtbo` to the Pi:

```bash
scp driver/build/bmp280_pi.ko bmp280-overlay.dtbo pi@<pi-ip>:~/
```

Then on the Pi, run the very simple loader script:

```bash
sudo ./load-bmp280.sh
```

The script loads the `regmap-i2c` and `industrialio` kernel dependencies, applies the device tree overlay so the kernel knows a BMP280 is on the I2C bus, and then inserts the driver module.

## Usage
 
Once the driver is loaded it registers an IIO device, usually at `/sys/bus/iio/devices/iio:device0/` or similar. Two read-only attributes are exposed:
 
```bash
# Temperature in millidegrees Celsius
cat /sys/bus/iio/devices/iio:device0/in_temp_input
 
# Pressure in kPa
cat /sys/bus/iio/devices/iio:device0/in_pressure_input
```
