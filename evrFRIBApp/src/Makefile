TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# Build on linux targets only
ifneq ($(filter linux-%,$(T_A)),)

DBD += evrFRIBSupport.dbd

LIBRARY_IOC += evrFRIB

evrFRIB_SRCS += evr_frib.cpp
evrFRIB_SRCS += evr_frib_iocsh.cpp

evrFRIB_LIBS += mrfCommon mrmShared evr epicspci
evrFRIB_LIBS += $(EPICS_BASE_IOC_LIBS)

endif

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

