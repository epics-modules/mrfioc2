TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the modular register map event receiver library

LIBRARY_IOC += evrMrm

evrMrm_SRCS += drvemIocsh.c
evrMrm_SRCS += drvem.cpp
evrMrm_SRCS += drvemOutput.cpp
evrMrm_SRCS += drvemInput.cpp
evrMrm_SRCS += drvemPrescaler.cpp
evrMrm_SRCS += drvemPulser.cpp
evrMrm_SRCS += drvemCML.cpp
evrMrm_SRCS += drvemRxBuf.cpp

evrMrm_SRCS += bufrxmgr.cpp

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

