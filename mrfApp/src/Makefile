TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = mrf
# mrf.dbd will be created and installed
DBD += mrf.dbd

# mrf.dbd will be made up from these files:
mrf_DBD += base.dbd
mrf_DBD += system.dbd

mrf_DBD += evgInit.dbd

mrf_DBD += drvemSupport.dbd

mrf_DBD += epicsvme.dbd
mrf_DBD += epicspci.dbd

ifdef BASE_7_0
mrf_DBD += evrSupportBase7.dbd
endif


# Add all the support libraries needed by this IOC
mrf_LIBS += evgmrm evrMrm evr mrmShared mrfCommon epicspci epicsvme

mrf_DBD += evrFRIBSupport.dbd
mrf_LIBS += evrFRIB

# mrf_registerRecordDeviceDriver.cpp derives from mrf.dbd
mrf_SRCS += mrf_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
mrf_SRCS_DEFAULT += mrfMain.cpp

ifneq ($(DEVIOCSTATS),)
mrf_DBD += devIocStats.dbd
mrf_LIBS += devIocStats
endif

ifneq ($(AUTOSAVE),)
mrf_DBD += asSupport.dbd
mrf_LIBS += autosave
endif

ifneq ($(GENERAL_TIME),)
mrf_DBD  += generalTime.dbd
mrf_LIBS += generalTime
endif

ifneq ($(CAPUTLOG),)
mrf_DBD += caPutLog.dbd
mrf_LIBS += caPutLog
endif

ifneq ($(PVA2PVA),)
mrf_DBD += PVAServerRegister.dbd
mrf_DBD += qsrv.dbd
mrf_LIBS += qsrv
mrf_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)
endif

# Finally link to the EPICS Base libraries
mrf_LIBS += $(EPICS_BASE_IOC_LIBS)

mrf_SYS_LIBS_WIN32 = ws2_32

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
