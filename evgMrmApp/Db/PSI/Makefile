TOP = ../../..
include $(TOP)/configure/CONFIG

DB += evgAcTrig.db
DB += evgDbus.db
DB += evgEvtClk.db
DB += evgInput.db
DB += evgMrm.db
DB += evgMxc.db
DB += evgOutput.db
DB += evgSoftEvt.db
DB += evgTrigEvt.db

DB += evgSoftSeq.template

DB += evg-vme.substitutions


ifneq ($(DEVIOCSTATS),)
ifeq ($(OS_CLASS), RTEMS)
DB_INSTALLS += $(EPICS_BASE)/db/iocAdminRTEMS.db
endif
endif

# Omitted before 3.14.10
USR_DBFLAGS += -I . -I .. -I $(INSTALL_LOCATION)/db

include $(TOP)/configure/RULES
