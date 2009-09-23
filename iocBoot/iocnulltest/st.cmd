#!../../bin/linux-x86/evrnull

< envPaths

cd ${TOP}

dbLoadDatabase "dbd/evrnull.dbd"
evrnull_registerRecordDeviceDriver pdbbase

addEVRNull(0,"EVR Test")

dbLoadRecords("db/evrbase.db","P=evr:,C=0")

cd ${TOP}/iocBoot/${IOC}
iocInit
