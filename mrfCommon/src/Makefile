TOP=../..
include $(TOP)/configure/CONFIG

USR_CFLAGS += -DDEBUG_PRINT
USR_CPPFLAGS += -DDEBUG_PRINT

MRF_VERSION = 2.0.4

#---------------------
# Install include files
#
INC += mrfBitOps.h
INC += mrfCommon.h        # Common MRF event system constants & definitions
INC += mrfCommonIO.h      # Common I/O access macros
INC += mrfFracSynth.h     # Fractional Synthesizer routines
INC += linkoptions.h
INC += mrfcsr.h

INC += mrf/databuf.h
INC += mrf/object.h

INC += mrf/version.h

INC += devObj.h


TESTPROD_HOST += objectTest
objectTest_SRCS += objectTest.c
objectTest_LIBS += mrfCommon $(EPICS_BASE_IOC_LIBS)
TESTS += objectTest

ifeq ($(EPICS_VERSION)$(EPICS_REVISION),314)
ifeq ($(findstring $(EPICS_MODIFICATION),1 2 3 4 5 6 7 8 9),)

# Test framework added in 3.14.10
TESTPROD_HOST += linkoptionsTest
linkoptionsTest_SRCS += linkoptionsTest.c
linkoptionsTest_LIBS += mrfCommon
TESTS += linkoptionsTest

else

# Added to Base in 3.14.10
DBDINC += aSubRecord
mrfCommon_SRCS += aSubRecord.c
endif
endif

#---------------------
# Install DBD files
#
DBD += mrfCommon.dbd

#---------------------
# Build the MRF Common Library
#
LIBRARY_IOC += mrfCommon
mrfCommon_SRCS += mrfFracSynth.c
mrfCommon_SRCS += linkoptions.c
mrfCommon_SRCS += object.cpp
mrfCommon_SRCS += devObjAnalog.cpp
mrfCommon_SRCS += devObjLong.cpp
mrfCommon_SRCS += devObjBinary.cpp
mrfCommon_SRCS += devObjMBB.cpp
mrfCommon_SRCS += devObjMBBDirect.cpp
mrfCommon_SRCS += devObjString.cpp
mrfCommon_SRCS += devObjWf.cpp
mrfCommon_SRCS += devMbboDirectSoft.c
mrfCommon_SRCS += databuf.cpp
mrfCommon_SRCS += mrfCommon.cpp

mrfCommon_LIBS += $(EPICS_BASE_IOC_LIBS)

#OBJS_IOC += $(mrfCommon_SRCS)
mrfCommon_LIBS += $(EPICS_BASE_IOC_LIBS)

#---------------------
# Build the host-side utilities for creating and analyzing
# the fractional synthesizer control words.
#
PROD_HOST += FracSynthAnalyze
PROD_HOST += FracSynthControlWord

FracSynthAnalyze_SRCS += FracSynthAnalyze.c
FracSynthControlWord_SRCS += FracSynthControlWord.c

PROD_LIBS += Com

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES

version.h::
	$(PERL) ../genVersionHeader.pl -t "$(TOP)" -V "$(MRF_VERSION)" -N MRF_VERSION $@

../O.Common/mrf/version.h: version.h
	$(INSTALL) -d $< ../O.Common/mrf/
