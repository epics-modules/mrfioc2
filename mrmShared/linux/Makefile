KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build

include CONFIG

MAKE += DRV_VERSION=$(DRV_VERSION)

all: modules

modules modules_install clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(CURDIR) $@

.PHONY: all modules modules_install clean
