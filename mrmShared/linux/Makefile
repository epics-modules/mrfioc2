
ifneq ($(KERNELRELEASE),)

  obj-m := mrf.o

  mrf-objs := uio_mrf.o jtag_mrf.o

else

  KERNELDIR ?= /lib/modules/$(shell uname -r)/build
  PWD := $(shell pwd)

default: modules ifc ppmac clean

modules modules_install clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@
        
myclean:
	rm -rf *.ko *.ko.unsigned *.o *.mod.* .*cmd .tmp_versions Module.symvers modules.order

endif

#########################################

ARCH_ifc = powerpc
CROSS_COMPILE_ifc = /opt/eldk-5.2/powerpc-e500v2/sysroots/i686-eldk-linux//usr/bin/ppce500v2-linux-gnuspe/powerpc-linux-gnuspe-
KERNELDIR_ifc = /opt/eldk-5.2/kernel/gfa-linux-3.6.11.5-rt37

ARCH_ppmac = powerpc
CROSS_COMPILE_ppmac = /opt/eldk-4.2/usr/bin/ppc_4xxFP-
KERNELDIR_ppmac = /opt/eldk-4.2/PPMAC_kernels/linux-2.6.30.3-xeno-2.5.6

ifc ppmac: myclean
	$(MAKE) ARCH=$(ARCH_$@) CROSS_COMPILE=$(CROSS_COMPILE_$@) KERNELDIR=$(KERNELDIR_$@) modules
	rm -rf $@
	mkdir $@
	mv mrf.ko $@
