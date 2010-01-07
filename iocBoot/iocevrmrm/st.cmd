
#< envPaths

cd("/mnt")

## Register all support components
dbLoadDatabase("dbd/evrmrm.dbd")
evrmrm_registerRecordDeviceDriver(pdbbase)

mrmEvrSetupPCI(0,1,2,0)

dbLoadRecords("db/evrbase.db","P=evr:,C=0")
dbLoadRecords("db/evrmap.db","P=evr:,M=blink,C=0,func=125,EVT=2")

dbLoadRecords("db/evrscale.db","P=evr:,O=scale1,C=0,IDX=0")

dbLoadRecords("db/mrmevrout.db","P=evr:,O=irqmap:,C=0,OT=0,IDX=0")
dbLoadRecords("db/mrmevrout.db","P=evr:,O=fp1:,C=0,OT=1,IDX=0")
dbLoadRecords("db/mrmevrout.db","P=evr:,O=fp2:,C=0,OT=1,IDX=1")
dbLoadRecords("db/mrmevrout.db","P=evr:,O=fp3:,C=0,OT=1,IDX=2")

dbLoadRecords("db/evrpulser.db","P=evr:,N=pul0:,C=0,PID=0")
dbLoadRecords("db/evrpulsermap.db","P=evr:,N=pul0:,M=m1,C=0,PID=0,F=1,EVT=2")

cd("${TOP}/iocBoot/${IOC}")
iocInit()
