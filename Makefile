#SHELL=cmd.exe
#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS := configure mrfCommon evrApp mrmShared evgMrmApp evrMrmApp mrmtestApp iocBoot

# 3.14.10 style directory dependencies
# previous versions will just ignore them

define DIR_template
 $(1)_DEPEND_DIRS = configure
endef
$(foreach dir, $(filter-out configure,$(DIRS)),$(eval $(call DIR_template,$(dir))))

iocBoot_DEPEND_DIRS += $(filter %App,$(DIRS))

evrApp_DEPEND_DIRS += mrfCommon mrmShared

mrmShared_DEPEND_DIRS += mrfCommon

evrMrmApp_DEPEND_DIRS += evrApp mrmShared

evgMrmApp_DEPEND_DIRS += mrmShared

mrmtestApp_DEPEND_DIRS += evrMrmApp evgMrmApp

include $(TOP)/configure/RULES_TOP


