TOP = ../../..
include $(TOP)/configure/CONFIG

DB += evr-delayModule.template
DB += evr-pulserMap.template
DB += evr-softEvent.template
DB += evr-specialFunctionMap.template

DB += evr-pcie-300.template
DB += evr-vmerf230.template

ifneq ($(DEVIOCSTATS),)
DB_INSTALLS += $(DEVIOCSTATS)/db/iocAdminRTEMS.db
endif

# Omitted before 3.14.10
USR_DBFLAGS += -I . -I .. -I $(INSTALL_LOCATION)/db

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
