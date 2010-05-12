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
DIRS := $(DIRS) $(filter-out $(DIRS), mrmtestApp)
DIRS := $(DIRS) $(filter-out $(DIRS), iocBoot)

include $(TOP)/configure/RULES_TOP


