#---------------------
# Set up the Environment
#
TOP=../..
include $(TOP)/configure/CONFIG

USR_CFLAGS += -DDEBUG_PRINT
USR_CPPFLAGS += -DDEBUG_PRINT

#---------------------
# Install include files
#
INC += evg/evg.h
INC += evg/Sequence.h
INC += drvEvg.h
INC += drvSequence.h

#---------------------
# Install DBD files
#
DBD += drvEvg.dbd

#---------------------
# Build the generic event generator library
#
LIBRARY_IOC += evg

evg_SRCS    += evg.cpp
evg_SRCS    += drvEvg.cpp
evg_SRCS    += drvSequence.cpp

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES
