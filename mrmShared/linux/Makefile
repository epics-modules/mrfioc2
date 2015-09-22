
ifneq ($(KERNELRELEASE),)

  obj-m := mrf.o

  mrf-objs := uio_mrf.o jtag_mrf.o

else

  KERNELDIR ?= /lib/modules/$(shell uname -r)/build
  PWD := $(shell pwd)

all: modules

modules modules_install clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@

.PHONY: all modules modules_install clean

endif
