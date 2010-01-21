#---------------------
# Set up the Environment
#
TOP=../..
include $(TOP)/configure/CONFIG

#---------------------
# Install DBD files
#
DBD += BasicSequence.dbd

#---------------------
# Build the basic sequencer library
#
LIBRARY_IOC += BasicSequence

BasicSequence_SRCS += BasicSequence.cpp
BasicSequence_SRCS += BasicSequenceEvent.cpp
BasicSequence_SRCS += devBasicSequence.cpp
BasicSequence_SRCS += devBasicSequenceEvent.cpp

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES

