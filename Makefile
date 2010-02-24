#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) $(filter-out $(DIRS), configure)
DIRS := $(DIRS) $(filter-out $(DIRS), mrfCommon)
DIRS := $(DIRS) $(filter-out $(DIRS), pciApp)
DIRS := $(DIRS) $(filter-out $(DIRS), vmeApp)
DIRS := $(DIRS) $(filter-out $(DIRS), evgApp)
DIRS := $(DIRS) $(filter-out $(DIRS), evgBasicSequenceApp)
DIRS := $(DIRS) $(filter-out $(DIRS), evgMrmApp)
DIRS := $(DIRS) $(filter-out $(DIRS), evrApp)
DIRS := $(DIRS) $(filter-out $(DIRS), evrMrmApp)
DIRS := $(DIRS) $(filter-out $(DIRS), iocBoot)



#DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *App))
#DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard iocBoot))
#
#define DIR_template
# $(1)_DEPEND_DIRS = configure
#endef
#$(foreach dir, $(filter-out configure,$(DIRS)),$(eval $(call DIR_template,$(dir))))
#
#iocBoot_DEPEND_DIRS += $(filter %App,$(DIRS))
#
#evgMrmApp_DEPEND_DIRS += pciApp vmeApp evgApp
#
#evrApp_DEPEND_DIRS += pciApp
#
#evrnullApp_DEPEND_DIRS += evrApp

include $(TOP)/configure/RULES_TOP


