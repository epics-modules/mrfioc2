#------------------------------------------------------------------------------
# $(TIMING)/mrfCommon/src/Makefile
#
#------------------------------------------------------------------------------
# Author:  Eric Bjorklund
# Date:    19 October 2009
#
#------------------------------------------------------------------------------
# Modification History:
# 19 Oct 2008  E.Bjorklund      Original Release
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
#   Build Host
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
# HOST_BUILD macro tells mrfFracSynth whether it should produce entries
# for the IOC shell
#
ifeq ($(EPICS_HOST_ARCH),$(T_A))
    USR_CFLAGS += -DHOST_BUILD
endif

#---------------------
# Only build for "hardware" IOC's
#
ifneq ($(findstring Host, $(VALID_BUILDS)),)
    VALID_BUILDS = Host
endif

#---------------------
# Install include files
#
INC += mrfIoOps.h
INC += mrfBitOps.h
INC += mrfCommon.h        # Common MRF event system constants & definitions
INC += mrfIoOps.h
INC += mrfCommonIO.h      # Common I/O access macros
INC += mrfBusInterface.h  # Hardware bus interface class definition
INC += debugPrint.h       # SLAC debug print utility

#---------------------
# Install DBD files
#
DBD += mrfCommon.dbd

#---------------------
# Build the MRF Common Library
#
LIBRARY_IOC += mrfCommon
mrfCommon_OBJS += mrfFracSynth
mrfCommon_OBJS += mrfBusInterface

OBJS_IOC += $(mrfCommon_OBJS)

#---------------------
# Build the host-side utilities for creating and analyzing
# the fractional synthesizer control words.
#
PROD_HOST += FracSynthAnalyze
PROD_HOST += FracSynthControlWord

FracSynthAnalyze_OBJS += FracSynthAnalyze
FracSynthAnalyze_OBJS += mrfFracSynth

FracSynthControlWord_OBJS += FracSynthControlWord
FracSynthControlWord_OBJS += mrfFracSynth

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES
