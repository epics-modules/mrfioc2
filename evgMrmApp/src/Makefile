TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

LIBRARY_IOC += evgmrm

INC += evgSeq/evgSeqManager.h
INC += evgSeq/evgSequence.h
INC += evgSeq/evgSeqRamManager.h
INC += evgSeq/evgSeqRam.h

SRC_DIRS += ../evgSeq

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

evgmrm_SRCS += evgInput.cpp
evgmrm_SRCS += devEvgInput.cpp

evgmrm_SRCS += evgOutput.cpp
evgmrm_SRCS += devEvgOutput.cpp

evgmrm_SRCS += evgSequence.cpp
evgmrm_SRCS += evgSeqManager.cpp
evgmrm_SRCS += devEvgSeq.cpp

evgmrm_SRCS += evgSeqRam.cpp
evgmrm_SRCS += evgSeqRamManager.cpp
evgmrm_SRCS += devEvgSeqRam.cpp

DBD += evgInit.dbd
#=============================
# Build the IOC application

PROD_IOC = evgMrm
# evgMrm.dbd will be created and installed
DBD += evgMrm.dbd

# evgMrm.dbd will be made up from these files:
evgMrm_DBD += base.dbd
evgMrm_DBD += evgInit.dbd

# Add all the support libraries needed by this IOC
evgMrm_LIBS += evgmrm vme mrfCommon

# evgMrm_registerRecordDeviceDriver.cpp derives from evgMrm.dbd
evgMrm_SRCS += evgMrm_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
evgMrm_SRCS_DEFAULT += evgMrmMain.cpp

#evgMrm_INCLUDES += -I/usr/powerpc-rtems4.9/mvme3100/lib/include
# Finally link to the EPICS Base libraries
evgMrm_LIBS += $(EPICS_BASE_IOC_LIBS)

ifneq ($(DEVIOCSTATS),)
evgMrm_DBD += devIocStats.dbd
evgMrm_LIBS += devIocStats
endif

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
