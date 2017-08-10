
TOP=../..
include $(TOP)/configure/CONFIG

INC += mrmDataBufTx.h
INC += mrmSeq.h
INC += mrmpci.h
INC += sfp.h

DBD += mrmShared.dbd

LIBRARY_IOC += mrmShared

# to avoid creating an mrfMrmCommon library
# when no non-MRM boards are supported yet
mrmShared_SRCS += mrmDataBufTx.cpp
mrmShared_SRCS += mrmSeq.cpp
mrmShared_SRCS += devMrfBufTx.cpp
mrmShared_SRCS += sfp.cpp
mrmShared_SRCS += mrmtimesrc.cpp

mrmShared_LIBS += mrfCommon $(EPICS_BASE_IOC_LIBS)

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES
