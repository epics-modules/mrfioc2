
TOP=../..
include $(TOP)/configure/CONFIG

USR_CPPFLAGS += -DBUILDING_MRMSHARED_API
USR_INCLUDES += -I$(TOP)/mrfCommon/src
USR_INCLUDES += -I$(TOP)/evrMrmApp/src

INC += mrmSharedAPI.h
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
mrmShared_SRCS += mrmspi.cpp

mrmShared_LIBS += mrfCommon $(EPICS_BASE_IOC_LIBS)

USR_SYS_LIBS_WIN32 += ws2_32
#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES
