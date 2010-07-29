
ifneq ($(KERNELRELEASE),)

  obj-m := mrf.o

  mrf-objs := uio_mrf.o

else

  KERNELDIR ?= /lib/modules/$(shell uname -r)/build
  PWD := $(shell pwd)

default: modules

modules modules_install clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@

endif





