
#< envPaths

cd("/mnt")

## Register all support components
dbLoadDatabase("dbd/evrmrm.dbd")
evrmrm_registerRecordDeviceDriver(pdbbase)

mrmEvrSetupPCI(0,1,2,0)

bspExtVerbosity=0
mrmEvrSetupVME(1,3,0x100000,4,0x24)

dbLoadRecords("db/evr-pmc-230.db","P=evr1:,C=0")

dbLoadRecords("db/evr-vmerf-230.db","P=evr2:,C=1")

cd("${TOP}/iocBoot/${IOC}")
iocInit()
