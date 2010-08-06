
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
mrmEvrSetupVME(1,5,0x20000000,3,0x26)
mrmEvrSetupVME(2,8,0x21000000,4,0x24)

dbLoadRecords("db/evr-pmc-230.db","P=evr1:,C=0")

dbLoadRecords("db/evr-vmerf-230.db","P=evr2:,C=1")
dbLoadRecords("db/evr-vmerf-230.db","P=evr3:,C=2")

dbLoadRecords("db/iocAdminRTEMS.db", "IOC=mrftest")

# Auto save/restore
set_savefile_path("${mnt}/as","/save")
set_requestfile_path("${mnt}/as","/req")

set_pass0_restoreFile("mrf_settings.sav")
set_pass0_restoreFile("mrf_values.sav")
set_pass1_restoreFile("mrf_values.sav")
set_pass1_restoreFile("mrf_waveforms.sav")

iocInit()

makeAutosaveFileFromDbInfo("as/req/mrf_settings.req", "autosaveFields_pass0")
makeAutosaveFileFromDbInfo("as/req/mrf_values.req", "autosaveFields")
makeAutosaveFileFromDbInfo("as/req/mrf_waveforms.req", "autosaveFields_pass1")

create_monitor_set("mrf_settings.req", 5 , "")
create_monitor_set("mrf_values.req", 5 , "")
create_monitor_set("mrf_waveforms.req", 30 , "")
