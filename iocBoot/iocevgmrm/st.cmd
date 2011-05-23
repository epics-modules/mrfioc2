
#!../../bin/linux-x86/nsls2evgMrm

## Register all support components
dbLoadDatabase("dbd/mrf.dbd")
mrf_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("ENGINEER","Jayesh Shah")
epicsEnvSet("LOCATION","Blg 902 Rm 28")

bspExtVerbosity=0

#mrmEvgSetupVME (
#    epicsInt32    cardNum,           // Logical card number
#    epicsInt32    slot,              // VME slot
#    epicsUInt32   vmeAddress,        // Desired VME address in A24 space
#    epicsInt32    irqLevel           // IRQ Level
#    epicsInt32    irqVector,         // Desired interrupt vector number
#)

mrmEvgSetupVME(1, 3, 0x100000, 4, 0xC0)

## Load record instances

dbLoadRecords("db/vme-evg230.db",   "S=TST,   D=evg:1, cardNum=1")

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

