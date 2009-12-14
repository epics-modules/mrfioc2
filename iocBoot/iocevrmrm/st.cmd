
#< envPaths

cd("/mnt")

## Register all support components
dbLoadDatabase("dbd/evrmrm.dbd")
evrmrm_registerRecordDeviceDriver(pdbbase)

mrmevrSetupAuto()

dbLoadRecords("db/evrbase.db","P=evr:,C=0")
#dbLoadRecords("db/evrmap.db","P=evr:,M=heartbeat:,C=0,func=7")
#dbLoadRecords("db/evrmap.db","P=evr:,M=resetscaler1:,C=0,func=8")
#dbLoadRecords("db/evrmap.db","P=evr:,M=resetscaler2:,C=0,func=8")

dbLoadRecords("db/evrscale.db","P=evr:,O=scale1,C=0,IDX=0")

dbLoadRecords("db/mrmevrout.db","P=evr:,O=irqmap:,C=0,OT=0,IDX=0")
dbLoadRecords("db/mrmevrout.db","P=evr:,O=fp1:,C=0,OT=1,IDX=0")
dbLoadRecords("db/mrmevrout.db","P=evr:,O=fp2:,C=0,OT=1,IDX=1")
dbLoadRecords("db/mrmevrout.db","P=evr:,O=fp3:,C=0,OT=1,IDX=2")

cd("${TOP}/iocBoot/${IOC}")
iocInit()
