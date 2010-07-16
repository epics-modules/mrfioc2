TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

LIBRARY = evr

SRC_DIRS += evr

DBD += evrSupport.dbd

INC += evr/util.h
INC += evr/pulser.h
INC += evr/output.h
INC += evr/input.h
INC += evr/prescaler.h
INC += evr/evr.h
INC += evr/cml.h

evr_SRCS += evr.cpp

INC += evrmap.h
evr_SRCS += evrGTIF.cpp

evr_SRCS += devEvr.cpp
evr_SRCS += devEvrStringIO.cpp

evr_SRCS += devEvrInput.cpp

evr_SRCS += devPreScaler.cpp
evr_SRCS += devEvrEvent.cpp
evr_SRCS += devEvrMapping.cpp
evr_SRCS += devEvrOutput.cpp

evr_SRCS += devEvrPulser.cpp
evr_SRCS += devEvrPulserMapping.cpp

evr_SRCS += devEvrCML.cpp

evr_SRCS += asub.c

evr_SRCS += evrSupport_registerRecordDeviceDriver.cpp

# Note: evrtest is NOT a working ioc.
#       it exists to ensure that libevr
#       will link correctly.
PROD_IOC += evrtest

evrtest_SRCS += evrMain.cpp

evrtest_LIBS += evr mrfCommon
evrtest_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

