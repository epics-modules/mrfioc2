TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the modular register map event receiver library

PROD_HOST += evrdump

evrdump_SRCS += evrdump.c
evrdump_LIBS += epicspci $(EPICS_BASE_IOC_LIBS)

ifeq ($(EPICS_VERSION)$(EPICS_REVISION),314)
ifneq ($(findstring $(EPICS_MODIFICATION),1 2 3 4 5 6 7 8 9),)
# libiocsh is merged with libCom starting with 3.14.10
evrdump_LIBS += iocsh
endif
endif

LIBRARY_IOC += evrMrm

evrMrm_SRCS += drvemIocsh.c
evrMrm_SRCS += drvem.cpp
evrMrm_SRCS += drvemOutput.cpp
evrMrm_SRCS += drvemInput.cpp
evrMrm_SRCS += drvemPrescaler.cpp
evrMrm_SRCS += drvemPulser.cpp
evrMrm_SRCS += drvemCML.cpp
evrMrm_SRCS += drvemRxBuf.cpp
evrMrm_SRCS += devMrmBuf.cpp

evrMrm_SRCS += bufrxmgr.cpp
evrMrm_SRCS += devMrmBufRx.cpp

evrMrm_SRCS += irqHack.cpp

evrMrm_SRCS += mrmGpio.cpp
#evrMrm_SRCS += delayModule.cpp

ifeq ($(OS),Windows_NT)
evrMrm_LIBS += mrfCommon mrmShared evr epicspci epicsvme $(EPICS_BASE_IOC_LIBS)
endif

#=============================
# Install the modular register map event receiver support dbd

DBD += drvemSupport.dbd

ifneq ($(DEVIOCSTATS),)
evrmrm_DBD += devIocStats.dbd
evrmrm_LIBS += devIocStats
endif

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

