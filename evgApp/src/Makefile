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
evg_SRCS    += drvEvg.cpp

#OBJS_IOC += $(evg_OBJS)

#---------------------
# Build the default sequencer library
#
LIBRARY_IOC      += evgSequence

evgSequence_SRCS += Sequence.cpp
evgSequence_SRCS += SequenceEvent.cpp

evgSequence_SRCS += devSequence.cpp
evgSequence_SRCS += devSequenceEvent.cpp

#OBJS_IOC += $(evgSequence_OBJS)

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES
