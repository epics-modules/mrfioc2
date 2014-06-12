include /ioc/tools/driver.makefile
EXCLUDE_VERSIONS=3.13.2 3.13.9 3.13.10 
#CROSS_COMPILER_TARGET_ARCHS=T2-ppc604
BUILDCLASSES = Linux
EXCLUDE_ARCHS += ppc405 xscale_be V6 SL5 syncTS 
#EXCLUDE_ARCHS += SL6 


USR_CPPFLAGS += -g \
	-I/afs/psi.ch/group/8431/slejko_t/CVS/F/TEST/SLEJKO/mrfioc2_PSI/mrfCommon/src \
	-I/afs/psi.ch/group/8431/slejko_t/CVS/F/TEST/SLEJKO/mrfioc2_PSI/evgMrmApp/src \
	-I/afs/psi.ch/group/8431/slejko_t/CVS/F/TEST/SLEJKO/mrfioc2_PSI/evrMrmApp/src  \
	-I/afs/psi.ch/group/8431/slejko_t/CVS/F/TEST/SLEJKO/mrfioc2_PSI/mrmShared/src \
	-I/afs/psi.ch/group/8431/slejko_t/CVS/F/TEST/SLEJKO/mrfioc2_PSI/evrApp/src
