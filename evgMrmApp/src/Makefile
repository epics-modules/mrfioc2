TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

USR_INCLUDES += -I$(TOP)/mrmShared/src

LIBRARY_IOC += evgmrm

INC += evgMrm.h
INC += evgRegMap.h
INC += evgAcTrig.h
INC += evgEvtClk.h
INC += evgTrigEvt.h
INC += evgMxc.h
INC += evgDbus.h
INC += evgInput.h
INC += evgOutput.h
INC += mrmevgseq.h

SRC_DIRS += ../devSupport 

#All the source files that are compiled and put in the library.
evgmrm_SRCS += evgInit.cpp

evgmrm_SRCS += evg.cpp

evgmrm_SRCS += evgMrm.cpp

evgmrm_SRCS += evgAcTrig.cpp

evgmrm_SRCS += evgEvtClk.cpp

evgmrm_SRCS += evgTrigEvt.cpp
evgmrm_SRCS += devEvgTrigEvt.cpp

evgmrm_SRCS += evgMxc.cpp

evgmrm_SRCS += evgDbus.cpp
evgmrm_SRCS += devEvgDbus.cpp

evgmrm_SRCS += evgInput.cpp

evgmrm_SRCS += evgOutput.cpp

evgmrm_SRCS += mrmevgseq.cpp

evgmrm_SRCS += seqconst.c
evgmrm_SRCS += seqnsls2.c

evgmrm_LIBS += mrfCommon mrmShared epicsvme epicspci $(EPICS_BASE_IOC_LIBS)

DBD += evgInit.dbd

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
