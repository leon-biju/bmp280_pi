KDIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/../rpi-linux)
ARCH := arm64
CROSS_COMPILE := aarch64-linux-gnu-

MODULE := bmp280_pi
obj-m := $(MODULE).o

BUILD := $(PWD)/build

all:
	@mkdir -p $(BUILD)
	$(MAKE) -C $(KDIR) M=$(PWD) MO=$(BUILD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) MO=$(BUILD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
	@rm -rf $(BUILD)