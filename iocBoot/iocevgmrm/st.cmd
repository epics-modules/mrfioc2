
#!../../bin/linux-x86/nsls2evgMrm

## Register all support components
dbLoadDatabase("dbd/mrf.dbd")
mrf_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("ENGINEER","Jayesh Shah")
epicsEnvSet("LOCATION","Blg 902 Rm 28")

bspExtVerbosity=0

#mrmEvgSetupVME (
#    const char*   id,                // EVG card ID
#    epicsInt32    slot,              // VME slot
#    epicsUInt32   vmeAddress,        // Desired VME address in A24 space
#    epicsInt32    irqLevel           // IRQ Level
#    epicsInt32    irqVector,         // Desired interrupt vector number
#)

mrmEvgSetupVME(EVG1, 3, 0x100000, 4, 0xC0)

## Load record instances

dbLoadRecords("db/vme-evg230.db", "SYS=TST, D=evg:1, EVG=EVG1")

# BNL specific timing sequence constructor
#dbLoadRecords("db/nsls2-inj-seqs.db","LN=LN-TS, BR=BR-TS, INJ=TST, EVG=evg:1, SEQ=SoftSeq:0")

dbLoadRecords("db/iocAdminRTEMS.db", "IOC=mrftest")

# Auto save/restore
save_restoreDebug(2)
dbLoadRecords("db/save_restoreStatus.db", "P=mrftest:")
save_restoreSet_status_prefix("mrftest:")

set_savefile_path("{mnt}/as","/save")
set_requestfile_path("{mnt}/as","/req")

set_pass0_restoreFile("mrf_settings.sav")
set_pass0_restoreFile("mrf_values.sav")
set_pass1_restoreFile("mrf_values.sav")
set_pass1_restoreFile("mrf_waveforms.sav")

iocInit()

makeAutosaveFileFromDbInfo("{mnt}/as/req/mrf_settings.req", "autosaveFields_pass0")
makeAutosaveFileFromDbInfo("{mnt}/as/req/mrf_values.req", "autosaveFields")
makeAutosaveFileFromDbInfo("{mnt}/as/req/mrf_waveforms.req", "autosaveFields_pass1")

create_monitor_set("mrf_settings.req", 10 , "")
create_monitor_set("mrf_values.req", 10 , "")
create_monitor_set("mrf_waveforms.req", 30 , "")

