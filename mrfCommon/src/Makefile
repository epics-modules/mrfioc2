#------------------------------------------------------------------------------
# $(TIMING)/mrfCommonApp/src/Makefile
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
#   APS Register Mask
#   Modular Register Mask
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
INC += mrfCommon.h        # Common MRF event system constants & definitions
INC += mrfCommonIO.h      # Common I/O access macros
INC += debugPrint.h       # SLAC debug print utility

#---------------------
# Install OS-Dependent include files
#
INC_vxWorks += mrfSyncIO.h
INC_RTEMS   += mrfSyncIO.h

#---------------------
# Build the MRF Common Library
#
LIBRARY_IOC += mrfCommon
mrfCommon_OBJS += mrfFracSynth

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
