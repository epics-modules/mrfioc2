KERNELDIR ?= /lib/modules/$(shell uname -r)/build

include CONFIG
include CONFIG_TARGET

all: modules

modules modules_install clean:
	$(MAKE) -C $(KERNELDIR) DRV_VERSION='$(DRV_VERSION)' M=$(CURDIR) CFLAGS_MODULE=$(CFLAGS_MODULE) $@

.PHONY: all modules modules_install clean
