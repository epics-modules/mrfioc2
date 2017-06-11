TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

DBD += evrFRIBSupport.dbd

LIBRARY_IOC += evrFRIB

evrFRIB_SRCS_Linux += evr_frib.cpp
evrFRIB_SRCS_Linux += evr_frib_iocsh.cpp

evrFRIB_SRCS_DEFAULT += dummy.c

evrFRIB_LIBS += mrfCommon mrmShared evr epicspci
evrFRIB_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

