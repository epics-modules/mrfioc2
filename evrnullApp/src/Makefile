TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

PROD_IOC = evrnull
# evrnull.dbd will be created and installed
DBD += evrnull.dbd

evrnull_DBD += base.dbd
evrnull_DBD += evrSupport.dbd
evrnull_DBD += nullreg.dbd

# evrnull_registerRecordDeviceDriver.cpp derives from evrnull.dbd
evrnull_SRCS += evrnull_registerRecordDeviceDriver.cpp

evrnull_SRCS += iocshfn.cpp
evrnull_SRCS += evrnull.cpp
evrnull_SRCS += outnull.cpp

# Build the main IOC entry point on workstation OSs.
evrnull_SRCS_DEFAULT += evrnullMain.cpp
evrnull_SRCS_vxWorks += -nil-

evrnull_LIBS += evr
evrnull_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

