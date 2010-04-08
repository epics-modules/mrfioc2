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

USR_CFLAGS += -DDEBUG_PRINT
USR_CPPFLAGS += -DDEBUG_PRINT

#---------------------
# Install include files
#
INC += evgMrm.h

#---------------------
# Install DBD files
#
DBD += evgMrm.dbd

evgMrm_DBD += mrfCommon.dbd
evgMrm_DBD += drvEvg.dbd
evgMrm_DBD += mrfVme.dbd
evgMrm_DBD += evgMrmConfig.dbd

#---------------------
# Build the modular register map event generator library
#
LIBRARY_IOC += evgMrm
evgMrm_SRCS += evgMrm.cpp

#OBJS_IOC += $(evgMrm_OBJS)

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES


