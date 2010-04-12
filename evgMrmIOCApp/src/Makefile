###############################################################################
# Makefile -- Create the IOC binaries and data base definition file
###############################################################################

TOP=../..
include $(TOP)/configure/CONFIG

#=======================================
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#======================================

#=============================
# Create the ioc application binary
#
PROD_IOC = evgMrmIOC

#---------------------
# Build the registration program, <name>_registerRecordDeviceDriver.cpp
# from the .dbd file.
#
evgMrmIOC_SRCS += evgMrmIOC_registerRecordDeviceDriver.cpp
evgMrmIOC_SRCS_DEFAULT += evgMrmIOCMain.cpp

#---------------------
# Add a definition for each additional object file to be built with this app.
#
evgMrmIOC_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

#---------------------
# Add a definition for other application libraries used by this app.
#
evgMrmIOC_LIBS += BasicSequence
evgMrmIOC_LIBS += Sequence
evgMrmIOC_LIBS += evgMrm
evgMrmIOC_LIBS += vme
evgMrmIOC_LIBS += evg
evgMrmIOC_LIBS += mrfCommon
evgMrmIOC_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================
# Create the application's dbd file
#
DBD += evgMrmIOC.dbd

evgMrmIOC_DBD += base.dbd
evgMrmIOC_DBD += evgMrm.dbd
evgMrmIOC_DBD += BasicSequence.dbd

#===========================
#  ADD RULES AFTER THIS LINE
#===========================

include $(TOP)/configure/RULES
