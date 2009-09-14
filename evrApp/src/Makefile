TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = evr
# evr.dbd will be created and installed
DBD += evr.dbd

# evr.dbd will be made up from these files:
evr_DBD += base.dbd

# Include dbd files from all support applications:
#evr_DBD += xxx.dbd

# Add all the support libraries needed by this IOC
#evr_LIBS += xxx

# evr_registerRecordDeviceDriver.cpp derives from evr.dbd
evr_SRCS += evr_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
evr_SRCS_DEFAULT += evrMain.cpp
evr_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#evr_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
evr_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

