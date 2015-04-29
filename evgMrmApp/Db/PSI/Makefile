TOP = ../../..
include $(TOP)/configure/CONFIG

DB += evgSoftSeq.template
DB += evg-vme.template

ifneq ($(DEVIOCSTATS),)
ifeq ($(OS_CLASS), RTEMS)
DB_INSTALLS += $(EPICS_BASE)/db/iocAdminRTEMS.db
endif
endif

# Omitted before 3.14.10
USR_DBFLAGS += -I . -I .. -I $(INSTALL_LOCATION)/db

include $(TOP)/configure/RULES
