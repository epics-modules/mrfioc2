TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the modular register map event receiver library

LIBRARY_IOC_vxWorks += evrMrm

evrMrm_SRCS += drvemIocsh.c
evrMrm_SRCS += drvem.cpp
evrMrm_SRCS += drvemOutput.cpp
evrMrm_SRCS += drvemInput.cpp
evrMrm_SRCS += drvemPrescaler.cpp
evrMrm_SRCS += drvemPulser.cpp
evrMrm_SRCS += drvemCMLShort.cpp

#=============================
# Install the modular register map event receiver support dbd

DBD += drvemSupport.dbd

#=============================
# Build the IOC application

PROD_IOC = evrmrm
# evrmrm.dbd will be created and installed
DBD += evrmrm.dbd

# evrmrm.dbd will be made up from these files:
evrmrm_DBD += base.dbd
evrmrm_DBD += drvemSupport.dbd
evrmrm_DBD += vmecsr.dbd

evrmrm_LIBS += pci vme evr mrfCommon
evrmrm_LIBS += $(EPICS_BASE_IOC_LIBS)

evrmrm_SRCS += $(evrMrm_SRCS)

# evrmrm_registerRecordDeviceDriver.cpp derives from evrmrm.dbd
evrmrm_SRCS += evrmrm_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
evrmrm_SRCS_DEFAULT += evrmrmMain.cpp
evrmrm_SRCS_vxWorks += -nil-

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

