
TOP=../..
include $(TOP)/configure/CONFIG

INC += mrmDataBufTx.h

LIBRARY_IOC += mrmShared

# to avoid creating an mrfMrmCommon library
# when no non-MRM boards are supported yet
mrmShared_SRCS += mrmDataBufTx.cpp


#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES
