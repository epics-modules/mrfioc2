include /ioc/tools/driver.makefile
EXCLUDE_VERSIONS=3.13.2 3.13.9 3.13.10 
BUILDCLASSES = Linux
USE_LIBVERSION=YES

USR_CPPFLAGS += -g \
        -I../../mrfioc2_PSI/mrfCommon/src \
	-I../../mrfioc2_PSI/evgMrmApp/src \
	-I../../mrfioc2_PSI/evrMrmApp/src  \
	-I../../mrfioc2_PSI/mrmShared/src \
	-I../../mrfioc2_PSI/evrApp/src
