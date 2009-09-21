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

evr_SRCS += evr.cpp

evr_SRCS += cardmap.cpp

evr_SRCS += devPreScaler.cpp


evr_SRCS += evrSupport_registerRecordDeviceDriver.cpp

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

