# bme280_pi
A driver and daemon for the BME280 sensor for the raspberry pi.


## Build Setup
### Kernel source

Clone the RPi kernel fork alongside the project and prepare it for module builds:

```bash
git clone --depth 1 --branch <branch> https://github.com/raspberrypi/linux.git rpi-linux

// Copy over .config from pi into rpi-linux

make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- olddefconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules_prepare

// Copy over Module.symvers as well
```

### Cross-compiler

We need the following dependencies: 

* aarch64-linux-gnu-gcc 
* make

### Build

    cd driver
    make

### Deploy

* Copy over the `hello.ko` file onto the pi.

* Then on the pi run:
```
sudo insmod ~/hello.ko
```

