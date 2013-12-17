include /ioc/tools/driver.makefile
EXCLUDE_VERSIONS=3.13.2 3.13.9 3.13.10 
#CROSS_COMPILER_TARGET_ARCHS=T2-ppc604
BUILDCLASSES = Linux
EXCLUDE_ARCHS += ppc405 xscale_be T2 V6 SL5 syncTS 
#EXCLUDE_ARCHS += SL6 

USR_CPPFLAGS += -g \
	-I/afs/psi.ch/user/s/slejko_t/workspace/mrfioc2202/mrfioc2202/mrfCommon/src \
	-I/afs/psi.ch/user/s/slejko_t/public/CVS/F/TEST/SLEJKO/mrfioc2202/evgMrmApp/src \
	-I/afs/psi.ch/user/s/slejko_t/public/CVS/F/TEST/SLEJKO/mrfioc2202/evrMrmApp/src  \
	-I/afs/psi.ch/user/s/slejko_t/public/CVS/F/TEST/SLEJKO/mrfioc2202/mrmShared/src \
	-I/afs/psi.ch/user/s/slejko_t/public/CVS/F/TEST/SLEJKO/mrfioc2202/evrApp/src
