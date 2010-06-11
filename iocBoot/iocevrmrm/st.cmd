
#< envPaths

cd("/mnt")

## Register all support components
dbLoadDatabase("dbd/mrf.dbd")
mrf_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("ENGINEER","mdavidsaver x3698")
epicsEnvSet("LOCATION","Blg 902 Rm 28")

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","1000000")

mrmEvrSetupPCI(0,1,2,0)

bspExtVerbosity=0
mrmEvrSetupVME(1,5,0x100000,3,0x26)
mrmEvrSetupVME(2,8,0x200000,4,0x24)

dbLoadRecords("db/evr-pmc-230.db","P=evr1:,C=0")

dbLoadRecords("db/evr-vmerf-230.db","P=evr2:,C=1")
dbLoadRecords("db/evr-vmerf-230.db","P=evr3:,C=2")

dbLoadRecords("db/iocAdminRTEMS.db", "IOC=mrftest")

iocInit()
