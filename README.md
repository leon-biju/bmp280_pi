# bmp280_pi

A custom Linux IIO kernel driver for the BMP280 sensor on the Raspberry Pi.

Note: A daemon will be implemented soon 

## Build Setup

### Kernel source

Clone the RPi kernel fork alongside the project and prepare it for out-of-tree module builds:

```bash
git clone --depth 1 --branch <branch> https://github.com/raspberrypi/linux.git rpi-linux

# Copy the running kernel's .config from the Pi into rpi-linux/
scp pi@<pi-ip>:/proc/config.gz . && gunzip config.gz && mv config rpi-linux/.config

make -C rpi-linux ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- olddefconfig
make -C rpi-linux ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules_prepare

# Copy Module.symvers from the Pi as well
scp pi@<pi-ip>:/lib/modules/$(ssh pi@<pi-ip> uname -r)/build/Module.symvers rpi-linux/
```

### Cross-compilation
If you are patient you can just build everything on the pi itself.

If you'd prefer to cross-compile install the following on your build machine:

- `aarch64-linux-gnu-gcc`
- `make`
- `dtc` (device-tree compiler)

### Build the driver

```bash
cd driver
make
```

### Build the device tree overlay

Compile the device tree source into a `.dtbo` overlay:

```bash
dtc -@ -I dts -O dtb -o bmp280-overlay.dtbo bmp280-overlay.dts
```

## Deploy

Copy `bmp280_pi.ko` and `bmp280-overlay.dtbo` to the Pi:

```bash
scp driver/bmp280_pi.ko bmp280-overlay.dtbo pi@<pi-ip>:~/
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