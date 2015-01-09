TOP = ../..
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *NSLS*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *nsls*))
include $(TOP)/configure/RULES_DIRS
