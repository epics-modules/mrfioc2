KERNELDIR ?= /lib/modules/$(shell uname -r)/build

include VERSION

all: modules

modules modules_install clean:
	$(MAKE) -C $(KERNELDIR) DRV_VERSION='$(DRV_VERSION)' M=$(CURDIR) $@

.PHONY: all modules modules_install clean
