TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = pci
# pci.dbd will be created and installed
DBD += pci.dbd

# pci.dbd will be made up from these files:
pci_DBD += base.dbd

# Include dbd files from all support applications:
#pci_DBD += xxx.dbd

# Add all the support libraries needed by this IOC
#pci_LIBS += xxx

# pci_registerRecordDeviceDriver.cpp derives from pci.dbd
pci_SRCS += pci_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
pci_SRCS_DEFAULT += pciMain.cpp
pci_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#pci_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
pci_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

