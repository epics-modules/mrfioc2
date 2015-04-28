TOP = ../../..
include $(TOP)/configure/CONFIG

DB += evr-base.template
DB += evr-cml.template
DB += evr-delayModule.template
DB += evr-in.template
DB += evr-out.template
DB += evr-prescaler.template
DB += evr-pulser.template
DB += evr-pulserMap.template
DB += evr-softEvent.template
DB += evr-specialFunctionMap.template
DB += sfp.template

DB += evr-pcie-300.substitutions
DB += evr-vmerf230.substitutions

ifneq ($(DEVIOCSTATS),)
DB_INSTALLS += $(DEVIOCSTATS)/db/iocAdminRTEMS.db
endif

# Omitted before 3.14.10
USR_DBFLAGS += -I . -I .. -I $(INSTALL_LOCATION)/db

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
