TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

LIBRARY_IOC = pci

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

