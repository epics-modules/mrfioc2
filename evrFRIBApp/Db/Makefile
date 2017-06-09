TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

USR_DBFLAGS += -I . -I .. -I $(INSTALL_LOCATION)/db

DB += fribevrout.db
DB += fribevrbase.db
DB += frib-fgpdb-evr.db

DB_INSTALLS += $(wildcard $(DEVLIB2)/db/frib-flash.db)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

