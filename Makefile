#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) $(filter-out $(DIRS), configure)
DIRS := $(DIRS) $(filter-out $(DIRS), mrfCommon)
DIRS := $(DIRS) $(filter-out $(DIRS), evgApp)
DIRS := $(DIRS) $(filter-out $(DIRS), evrApp)
DIRS := $(DIRS) $(filter-out $(DIRS), mrmShared)
DIRS := $(DIRS) $(filter-out $(DIRS), evgMrmApp)
DIRS := $(DIRS) $(filter-out $(DIRS), evgBasicSequenceApp)
DIRS := $(DIRS) $(filter-out $(DIRS), evrMrmApp)
DIRS := $(DIRS) $(filter-out $(DIRS), mrmtestApp)
DIRS := $(DIRS) $(filter-out $(DIRS), iocBoot)

# 3.14.10 style directory dependencies
# previous versions will just ignore them

define DIR_template
 $(1)_DEPEND_DIRS = configure
endef
$(foreach dir, $(filter-out configure,$(DIRS)),$(eval $(call DIR_template,$(dir))))

iocBoot_DEPEND_DIRS += $(filter %App,$(DIRS))

evrApp_DEPEND_DIRS += mrfCommon
evgApp_DEPEND_DIRS += mrfCommon
evgBasicSequenceApp_DEPEND_DIRS += mrfCommon evgApp

mrmShared_DEPEND_DIRS += mrfCommon

evrMrmApp_DEPEND_DIRS += evrApp mrmShared

evgMrmApp_DEPEND_DIRS += evgApp mrmShared

mrmtestApp_DEPEND_DIRS += evrMrmApp evgMrmApp

include $(TOP)/configure/RULES_TOP


