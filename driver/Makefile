KDIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/../rpi-linux)
ARCH := arm64
CROSS_COMPILE := aarch64-linux-gnu-

obj-m := driver.o

all:
	make -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	make -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
