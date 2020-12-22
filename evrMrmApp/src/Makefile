TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

USR_CPPFLAGS += -DBUILDING_EVRMRM_API
USR_INCLUDES += -I$(TOP)/mrmShared/src
USR_INCLUDES += -I$(TOP)/mrfCommon/src

#=============================
# Build the modular register map event receiver library

PROD_HOST += evrdump

evrdump_SRCS += evrdump.c
evrdump_LIBS += epicspci $(EPICS_BASE_IOC_LIBS)

LIBRARY_IOC += evrMrm

evrMrm_SRCS += drvemIocsh.cpp
evrMrm_SRCS += drvemSetup.cpp
evrMrm_SRCS += drvem.cpp
evrMrm_SRCS += drvemOutput.cpp
evrMrm_SRCS += drvemInput.cpp
evrMrm_SRCS += drvemPrescaler.cpp
evrMrm_SRCS += drvemPulser.cpp
evrMrm_SRCS += drvemCML.cpp
evrMrm_SRCS += drvemTSBuffer.cpp
evrMrm_SRCS += delayModule.cpp
evrMrm_SRCS += drvemRxBuf.cpp
evrMrm_SRCS += devMrmBuf.cpp

evrMrm_SRCS += mrmevrseq.cpp

evrMrm_SRCS += bufrxmgr.cpp
evrMrm_SRCS += devMrmBufRx.cpp

evrMrm_SRCS += irqHack.cpp

evrMrm_SRCS += mrmGpio.cpp

evrMrm_LIBS += mrfCommon mrmShared evr epicspci epicsvme $(EPICS_BASE_IOC_LIBS)

#=============================
# Install the modular register map event receiver support dbd

DBD += drvemSupport.dbd

USR_SYS_LIBS_WIN32 += ws2_32
ifneq ($(DEVIOCSTATS),)
evrmrm_DBD += devIocStats.dbd
evrmrm_LIBS += devIocStats
endif

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
