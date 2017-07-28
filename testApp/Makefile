TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

ifdef BASE_3_15

TARGETS += $(COMMON_DIR)/testmrf.dbd
DBDDEPENDS_FILES += testmrf.dbd$(DEP)

testmrf_DBD += base.dbd
testmrf_DBD += mrfCommon.dbd

TESTPROD_HOST += testlut
testlut_SRCS += testlut.c
testlut_SRCS += testmrf_registerRecordDeviceDriver.cpp
TESTS += testlut

PROD_LIBS += mrfCommon
PROD_LIBS += $(EPICS_BASE_IOC_LIBS)

TESTSCRIPTS_HOST += $(TESTS:%=%.t)

endif # BASE_3_15

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

