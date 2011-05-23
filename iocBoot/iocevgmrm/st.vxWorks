
#=====================
# Setup command line editing
#
tyBackspaceSet(0177)

#=====================
# Load the EPICS application.
#
cd timingbin
ld < evgMrmIOC.munch

#=====================
# Configure the timing cards
#
EgConfigureVME (0, 10, 0x240000, 0xC0, 4)
EgDebug (0, 3)

#=====================
# Register all support components
#
cd timing
dbLoadDatabase("dbd/evgMrmIOC.dbd",0,0)
evgMrmIOC_registerRecordDeviceDriver(pdbbase)

#=====================
# Load the master database
#
dbLoadRecords("db/evgMrmIOC.db")

#=====================
# Start the IOC
#
iocInit()
