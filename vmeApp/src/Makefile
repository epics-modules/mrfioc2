#------------------------------------------------------------------------------
# $(TIMING)/vmeApp/src/Makefile
#
#------------------------------------------------------------------------------
# Author:  Eric Bjorklund
# Date:    20 October 2009
#
#------------------------------------------------------------------------------
# Modification History:
# 20 Oct 2008  E.Bjorklund      Original Release
#
#------------------------------------------------------------------------------
# FILE DESCRIPTION:
#   This file contains the EPICS make rules and definitions for making and
#   installing common utilities and definitions for the Micro-Research Finland
#   event system.
#
#------------------------------------------------------------------------------
# Hardware Supported:
#   Series 2xx Event Generator and Event Receiver Cards
#     APS Register Mask
#     Modular Register Mask
#
#------------------------------------------------------------------------------
# Operating Systems Supported:
#   vxWorks
#   RTEMS
#
#------------------------------------------------------------------------------

#---------------------
# Set up the Environment
#
TOP=../..
include $(TOP)/configure/CONFIG

USR_CPPFLAGS += -DDEBUG_PRINT

#---------------------
# Install include files
#
INC += mrfVmeBusInterface.h
INC += mrfcsr.h
INC += devcsr.h
INC += vmedefs.h

#---------------------
# Install DBD files
#
DBD += mrfVme.dbd
DBD += vmecsr.dbd

#---------------------
# Build the VME Common Library
#
LIBRARY_IOC += vme

vme_SRCS += devcsr.c
vme_SRCS += iocreg.c
vme_LIBS += Com

LIBRARY_IOC += mrfVme

mrfVme_LIBS += Com
#mrfVme_SRCS += mrfVme64x  #~~~#
mrfVme_SRCS += mrfVmeBusInterface.cpp

#mrfVme_OBJS += vmeCSRMemProbeOSI #~~~~~#

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES

