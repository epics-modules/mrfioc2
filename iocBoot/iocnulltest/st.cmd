#!../../bin/linux-x86/evrnull

< envPaths

cd ${TOP}

dbLoadDatabase "dbd/evrnull.dbd"
evrnull_registerRecordDeviceDriver pdbbase

addEVRNull(0,"EVR Test")

dbLoadRecords("db/evrbase.db","P=evr:,C=0")
dbLoadRecords("db/evrmap.db","P=evr:,M=heartbeat,C=0,func=7")
dbLoadRecords("db/evrmap.db","P=evr:,M=resetscaler1,C=0,func=8")
dbLoadRecords("db/evrmap.db","P=evr:,M=resetscaler2,C=0,func=8")

dbLoadRecords("db/evrout.db","P=evr:,O=out1,C=0,id=268")
dbLoadRecords("db/evrout.db","P=evr:,O=out2,C=0,id=269")
dbLoadRecords("db/evrout.db","P=evr:,O=out3,C=0,id=210")
dbLoadRecords("db/evrout.db","P=evr:,O=out4,C=0,id=211")

cd ${TOP}/iocBoot/${IOC}
iocInit
