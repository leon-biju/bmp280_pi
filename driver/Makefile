CROSS_COMPILE ?=

MODULE := bmp280_pi
obj-m := $(MODULE).o

BUILD := $(PWD)/build

ifeq ($(CROSS_COMPILE),)
  # native build (e.g. on the Pi itself), against the running kernel
  KDIR ?= /lib/modules/$(shell uname -r)/build
else
  # cross build against the checked-out Pi kernel tree
  ARCH ?= arm64
  KDIR ?= $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/../rpi-linux)
endif

all:
	@mkdir -p $(BUILD)
	$(MAKE) -C $(KDIR) M=$(PWD) MO=$(BUILD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) MO=$(BUILD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
	@rm -rf $(BUILD)