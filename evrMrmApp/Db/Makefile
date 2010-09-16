TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

DB += mrmevrout.db
DB += mrmevrbufrx.db
DB += sequencedemo.db

DB += evr-pmc-230.db
DB += evr-vmerf-230.db

ifneq ($(DEVIOCSTATS),)
DB_INSTALLS += $(DEVIOCSTATS)/db/iocAdminRTEMS.db
endif

# Omitted before 3.14.10
USR_DBFLAGS += -I $(INSTALL_LOCATION)/db

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

