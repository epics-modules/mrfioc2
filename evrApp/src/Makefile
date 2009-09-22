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
INC += evr/prescaler.h
INC += evr/evr.h

evr_SRCS += evr.cpp

INC += cardmap.h
evr_SRCS += cardmap.cpp

evr_SRCS += devPreScaler.cpp
evr_SRCS += devEvr.cpp


evr_SRCS += evrSupport_registerRecordDeviceDriver.cpp

# Note: evrtest is NOT a working ioc.
#       it exists to ensure that libevr
#       will link correctly.
PROD_IOC += evrtest

evrtest_SRCS += evrMain.cpp

evrtest_LIBS += evr
evrtest_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

