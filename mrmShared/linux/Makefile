KERNELDIR ?= /lib/modules/$(shell uname -r)/build

include CONFIG
include CONFIG_TARGET

MAKE += DRV_VERSION=$(DRV_VERSION)
MAKE += CFLAGS_TARGET=$(CFLAGS_TARGET)

all: modules

modules modules_install clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURDIR) $@

.PHONY: all modules modules_install clean
