TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

USR_CFLAGS += -Wall -Werror

LIBRARY_RTEMS = pci

SRCS_DIRS += os/RTEMS
USR_CPPFLAGS += -I../os/RTEMS

INC += devLibPCI.h
INC += devLibPCIImpl.h
pci_SRCS += devLibPCI.c

INC_RTEMS += osdPCI.h
pci_SRCS_RTEMS += osdPCI.c

pci_LIBS += Com

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

