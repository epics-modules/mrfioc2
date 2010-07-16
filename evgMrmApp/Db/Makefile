TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

#----------------------------------------------------
# Create and install (or just install) into <top>/db
# databases, templates, substitutions like this
#DB += xxx.db
DB += evgMrm.db
DB += evgMxc.db
DB += evgTrigEvt.db
DB += evgDbus.db
DB += evgInput.db
DB += evgOutput.db
DB += evgSeq.db
DB += evgSeqRam.db
DB += vme-evg230.db


ifneq ($(DEVIOCSTATS),)
ifeq ($(OS_CLASS), RTEMS)
DB_INSTALLS += $(EPICS_BASE)/db/iocAdminRTEMS.db
endif
endif

#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_template = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

