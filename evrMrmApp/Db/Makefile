TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

USR_DBFLAGS += -I$(TOP)/mrmShared/Db

DB += mrmevrout.db
DB += mrmevrbase.template
DB += mrmevrdc.template
DB += mrmevrbufrx.db
DB += mrmevrtsbuf.db
DB += sequencedemo.db
DB += mrmevrdlymodule.template

DB += evr-pmc-230.db
DB += evr-cpci-230.db
DB += evr-cpci-300.db
DB += evr-vmerf-230.db
DB += evr-tg-300.db
DB += evr-mtca-300.db
DB += evr-pcie-300dc.db

ifdef BASE_3_15
DB += evr-mtca-300u.db
endif

ifneq ($(DEVIOCSTATS),)
DB_INSTALLS += $(DEVIOCSTATS)/db/iocAdminRTEMS.db
DB_INSTALLS += $(DEVIOCSTATS)/db/iocAdminSoft.db
endif

USR_DBFLAGS += -I . -I .. -I $(INSTALL_LOCATION)/db

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
