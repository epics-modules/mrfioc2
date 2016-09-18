
TOP=../..
include $(TOP)/configure/CONFIG

INC += plx9030.h
INC += plx9056.h
INC += latticeEC30.h
INC += mrmDataBufTx.h
INC += mrmpci.h
INC += sfp.h
INC += configurationInfo.h

DBD += mrmShared.dbd

LIBRARY_IOC += mrmShared

# to avoid creating an mrfMrmCommon library
# when no non-MRM boards are supported yet
mrmShared_SRCS += mrmDataBufTx.cpp
mrmShared_SRCS += devMrfBufTx.cpp
mrmShared_SRCS += sfp.cpp

ifeq ($(OS),Windows_NT)
mrmShared_LIBS += mrfCommon $(EPICS_BASE_IOC_LIBS)
mrmShared_SYS_LIBS += WS2_32
endif

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES
