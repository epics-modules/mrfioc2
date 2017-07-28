#SHELL=cmd.exe
#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS := configure mrfCommon evrApp mrmShared evgMrmApp evrMrmApp mrfApp evrFRIBApp testApp iocBoot

define DIR_template
 $(1)_DEPEND_DIRS = configure
endef
$(foreach dir, $(filter-out configure,$(DIRS)),$(eval $(call DIR_template,$(dir))))

iocBoot_DEPEND_DIRS += $(filter %App,$(DIRS))

evrApp_DEPEND_DIRS += mrfCommon

mrmShared_DEPEND_DIRS += mrfCommon

evrMrmApp_DEPEND_DIRS += evrApp mrmShared

evgMrmApp_DEPEND_DIRS += mrmShared

evrFRIBApp_DEPEND_DIRS += evrApp

mrfApp_DEPEND_DIRS += evrMrmApp evgMrmApp evrFRIBApp

testApp_DEPEND_DIRS += mrfCommon

include $(TOP)/configure/RULES_TOP
