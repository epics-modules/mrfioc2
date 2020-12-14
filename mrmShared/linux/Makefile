
  KERNELDIR ?= /lib/modules/$(shell uname -r)/build
  PWD := $(shell pwd)

all: modules

modules modules_install clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@

.PHONY: all modules modules_install clean
