TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

LIBRARY_IOC = pci

USR_INCLUDES += -I../os/$(OS_CLASS)

DBD += devlibpci.dbd

INC += devLibPCI.h
INC += devLibPCIImpl.h

pci_SRCS += devLibPCI.c

pci_SRCS_RTEMS += osdPciShared.c
pci_SRCS_vxWorks += osdPciShared.c

pci_SRCS += devLibPCIOSD.c

pci_LIBS += Com

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

