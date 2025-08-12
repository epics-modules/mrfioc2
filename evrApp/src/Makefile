TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

LIBRARY = evr

SRC_DIRS += evr

USR_INCLUDES += -I$(TOP)/mrfCommon/src

DBD += evrSupport.dbd
ifdef BASE_7_0
DBD += evrSupportBase7.dbd
endif

# INC += evr/pulser.h
# INC += evr/output.h
# INC += evr/delay.h
# INC += evr/input.h
# INC += evr/prescaler.h
# INC += evr/evr.h
# INC += evr/cml.h

evr_SRCS += evr.cpp

# INC += evrGTIF.h
evr_SRCS += evrGTIF.cpp

evr_SRCS += devEvrStringIO.cpp

evr_SRCS += devEvrEvent.cpp
evr_SRCS += devEvrMapping.cpp

evr_SRCS += devEvrPulserMapping.cpp

evr_SRCS += asub.c
evr_SRCS += devWfMailbox.c


evr_SRCS_Linux += ntpShm.cpp
evr_SRCS_DEFAULT += ntpShmNull.cpp

evr_LIBS += mrfCommon $(EPICS_BASE_IOC_LIBS)

evr_CPPFLAGS += -D__COMMIT_HASH=\"$(shell git rev-parse --short=8 HEAD 2>/dev/null || echo unknown)\"

# Note: evrtest is NOT a working ioc.
#       it exists to ensure that libevr
#       will link correctly.
PROD_IOC += evrtest

evrtest_SRCS += evrMain.cpp
evrtest_SRCS += evrtest_registerRecordDeviceDriver.cpp
evrtest_LIBS += evr mrfCommon

evrtest_DBD += base.dbd
evrtest_DBD += evrSupport.dbd
ifdef BASE_7_0
evrtest_DBD += evrSupportBase7.dbd
endif

ifneq ($(GENERAL_TIME),)
evrtest_LIBS += generalTime
evrtest_DBD  += generalTime.dbd
endif

evrtest_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
