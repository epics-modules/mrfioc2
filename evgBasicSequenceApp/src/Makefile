TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC += evgBasicSequence

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# install evgBasicSequence.dbd into <top>/dbd
DBD += evgBasicSequence.dbd

# specify all source files to be compiled and added to the library
#evgBasicSequence_SRCS += xxx

evgBasicSequence_LIBS += $(EPICS_BASE_IOC_LIBS)

#==================================================
# Install all protocol files in the data directory

DATA += $(patsubst ../%, %, $(wildcard ../*.proto))

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

