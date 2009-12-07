TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = evrmrm
# evrmrm.dbd will be created and installed
DBD += evrmrm.dbd

# evrmrm.dbd will be made up from these files:
evrmrm_DBD += base.dbd
evrmrm_DBD += evrmrmSupport.dbd

evrmrm_LIBS += pci evr mrfCommon
evrmrm_LIBS += $(EPICS_BASE_IOC_LIBS)

evrmrm_SRCS += pcisearch.c
evrmrm_SRCS += evrmrmiocsh.c
evrmrm_SRCS += evrmrm.cpp
evrmrm_SRCS += evrmrmoutput.cpp
evrmrm_SRCS += evrmrmprescaler.cpp

# evrmrm_registerRecordDeviceDriver.cpp derives from evrmrm.dbd
evrmrm_SRCS += evrmrm_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
evrmrm_SRCS_DEFAULT += evrmrmMain.cpp
evrmrm_SRCS_vxWorks += -nil-

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

