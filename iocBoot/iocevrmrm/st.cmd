
#< envPaths

cd("/mnt")

## Register all support components
dbLoadDatabase("dbd/evrmrm.dbd")
evrmrm_registerRecordDeviceDriver(pdbbase)

mrmEvrSetupPCI(0,1,2,0)

dbLoadRecords("db/evr-pmc-230.db","P=evr:,C=0")

cd("${TOP}/iocBoot/${IOC}")
iocInit()
