TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC_RTEMS = mrf
# mrf.dbd will be created and installed
DBD += mrf.dbd

# mrf.dbd will be made up from these files:
mrf_DBD += base.dbd

mrf_DBD += evgInit.dbd

mrf_DBD += drvemSupport.dbd

mrf_DBD += vmecsr.dbd
mrf_DBD += devIocStats.dbd

# Add all the support libraries needed by this IOC
mrf_LIBS += evgmrm evrMrm mrmShared pci vme evr mrfCommon devIocStats

# mrf_registerRecordDeviceDriver.cpp derives from mrf.dbd
mrf_SRCS += mrf_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
mrf_SRCS_DEFAULT += mrfMain.cpp

# Finally link to the EPICS Base libraries
mrf_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE