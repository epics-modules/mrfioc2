TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

LIBRARY_IOC = pci

ifeq ($(OS_CLASS),RTEMS)
SRCS_DIRS += os/RTEMS
USR_CPPFLAGS += -I../os/RTEMS

else ifeq ($(OS_CLASS),vxWorks)
SRCS_DIRS += os/vxWorks
USR_CPPFLAGS += -I../os/vxWorks

else
SRCS_DIRS += os/default
USR_CPPFLAGS += -I../os/default

endif

INC += devLibPCI.h
INC += devLibPCIImpl.h
INC += pciEndian.h
pci_SRCS += devLibPCI.c

pci_SRCS += osdPCI.c

pci_LIBS += Com

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

