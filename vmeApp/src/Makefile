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

#---------------------
# Only build for "hardware" IOC's
#
ifneq ($(findstring Host, $(VALID_BUILDS)),)
    VALID_BUILDS = Host
endif

#---------------------
# Build the VME Common Library
#
LIBRARY_IOC += mrfVme
mrfVme_OBJS += mrfVme64x
mrfVme_OBJS += mrfVmeBusInterface

OBJS_IOC += $(mrfVme_OBJS)

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES

