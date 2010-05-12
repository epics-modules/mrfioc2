TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

LIBRARY_IOC += evgmrm

#All the source files that are compiled and put in the library.
evgmrm_SRCS += evgInit.cpp

evgmrm_SRCS += evgMrm.cpp
evgmrm_SRCS += devEvgMrm.cpp

evgmrm_SRCS += evgMxc.cpp
evgmrm_SRCS += devEvgMxc.cpp

evgmrm_SRCS += evgTrigEvt.cpp
evgmrm_SRCS += devEvgTrigEvt.cpp

evgmrm_SRCS += evgDbus.cpp
evgmrm_SRCS += devEvgDbus.cpp

evgmrm_SRCS += evgFPio.cpp
evgmrm_SRCS += devEvgFPio.cpp

DBD += evgInit.dbd
#=============================
# Build the IOC application

PROD_IOC = evgMrm
# evgMrm.dbd will be created and installed
DBD += evgMrm.dbd

# evgMrm.dbd will be made up from these files:
evgMrm_DBD += base.dbd
evgMrm_DBD += evgInit.dbd
evgMrm_DBD += devIocStats.dbd

# Add all the support libraries needed by this IOC
evgMrm_LIBS += evgmrm vme mrfCommon devIocStats

# evgMrm_registerRecordDeviceDriver.cpp derives from evgMrm.dbd
evgMrm_SRCS += evgMrm_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
evgMrm_SRCS_DEFAULT += evgMrmMain.cpp

# Finally link to the EPICS Base libraries
evgMrm_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
