TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

USR_DBFLAGS += -I$(TOP)/mrmShared/Db
USR_DBFLAGS +=  -I$(TOP)/evrMrmApp/Db

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

DB += nsls2-inj-seqs.db

#----------------------------------------------------
# Create and install (or just install) into <top>/db
# databases, templates, substitutions like this

DB += evgMrm.db
DB += evgEvtClk.db
DB += evgUserEvt.db
DB += evgTrigEvt.db
DB += evgMxc.db
DB += evgDbus.db
DB += evgAcTrig.db
DB += evgInput.db
DB += evgOutput.db
DB += evgSoftSeq.db
DB += cpci-evg-300.db
DB += vme-evg230.db
DB += vme-evg230-nsls2.db
DB += mtca-evm-300.db

ifneq ($(DEVIOCSTATS),)
ifeq ($(OS_CLASS), RTEMS)
DB_INSTALLS += $(DEVIOCSTATS)/db/iocAdminRTEMS.db
endif
endif

USR_DBFLAGS += -I . -I .. -I $(INSTALL_LOCATION)/db

#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_template = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

