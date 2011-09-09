TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

LIBRARY_IOC += evgmrm

INC += evgSequencer/evgSoftSeqManager.h
INC += evgSequencer/evgSoftSeq.h
INC += evgSequencer/evgSeqRamManager.h
INC += evgSequencer/evgSeqRam.h

SRC_DIRS += ../evgSequencer ../devSupport 

#All the source files that are compiled and put in the library.
evgmrm_SRCS += evgInit.cpp

evgmrm_SRCS += evg.cpp

evgmrm_SRCS += evgMrm.cpp
evgmrm_SRCS += devEvgMrm.cpp

evgmrm_SRCS += evgAcTrig.cpp

evgmrm_SRCS += evgEvtClk.cpp

evgmrm_SRCS += evgSoftEvt.cpp

evgmrm_SRCS += evgTrigEvt.cpp
evgmrm_SRCS += devEvgTrigEvt.cpp

evgmrm_SRCS += evgMxc.cpp

evgmrm_SRCS += evgDbus.cpp
evgmrm_SRCS += devEvgDbus.cpp

evgmrm_SRCS += evgInput.cpp

evgmrm_SRCS += evgOutput.cpp

evgmrm_SRCS += evgBunchTrigger.c

evgmrm_SRCS += evgSoftSeq.cpp
evgmrm_SRCS += evgSoftSeqManager.cpp
evgmrm_SRCS += devEvgSoftSeq.cpp

evgmrm_SRCS += evgSeqRam.cpp
evgmrm_SRCS += evgSeqRamManager.cpp

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
evgMrm_LIBS += evgmrm epicsvme mrmShared mrfCommon 

# evgMrm_registerRecordDeviceDriver.cpp derives from evgMrm.dbd
evgMrm_SRCS += evgMrm_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
evgMrm_SRCS_DEFAULT += evgMrmMain.cpp

ifneq ($(DEVIOCSTATS),)
evgMrm_DBD += devIocStats.dbd
evgMrm_LIBS += devIocStats
endif

ifneq ($(AUTOSAVE),)
evgMrm_DBD += asSupport.dbd
evgMrm_LIBS += autosave
endif

ifneq ($(GENERAL_TIME),)
evgMrm_DBD  += generalTime.dbd
evgMrm_LIBS += generalTime
endif

# Finally link to the EPICS Base libraries
evgMrm_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
