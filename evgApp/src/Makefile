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
INC += drvEvg.h
INC += evg/evg.h

#---------------------
# Install DBD files
#
DBD += drvEvg.dbd

#---------------------
# Build the generic event generator library
#
LIBRARY_IOC += evg
evg_SRCS    += evg.cpp
evg_OBJS    += drvEvg.cpp

#OBJS_IOC += $(evg_OBJS)

#---------------------
# Build the default sequencer library
#
LIBRARY_IOC      += evgSequence

evgSequence_OBJS += Sequence
evgSequence_OBJS += SequenceEvent

evgSequence_OBJS += devSequence
evgSequence_OBJS += devSequenceEvent

OBJS_IOC += $(evgSequence_OBJS)

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES
