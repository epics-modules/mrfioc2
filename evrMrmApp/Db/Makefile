TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

DB += mrmevrout.db
DB += mrmevrbufrx.db

DB += evr-pmc-230.db
DB += evr-vmerf-230.db

ifneq ($(DEVIOCSTATS),)
DB_INSTALLS += $(DEVIOCSTATS)/db/iocAdminRTEMS.db
endif

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

