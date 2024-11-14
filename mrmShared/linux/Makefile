KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build

include CONFIG

all: modules

modules modules_install clean:
	$(MAKE) -C $(KERNEL_SRC) DRV_VERSION='$(DRV_VERSION)' M=$(CURDIR) $@

.PHONY: all modules modules_install clean
