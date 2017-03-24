#!../../bin/linux-x86_64/mrf

## Register all support components
dbLoadDatabase("../../dbd/mrf.dbd")
mrf_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("ENGINEER","mdavidsaver x608")
epicsEnvSet("LOCATION","FE-001.05")

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","10000000")

fribEvrSetupPCI("EVR", "slot=2-1")

dbLoadRecords("../../db/frib-fgpdb-evr.db","SYS=DIAG_MTCA, D=EVR1, EVR=EVR")

dbLoadRecords("../../frib-flash.db","P=DIAG_MTCA:EVR1-FLSH:,DEV=slot=2-1,location=0x000000,NELM=8388608")

dbLoadRecords("../../db/iocAdminSoft.db", "IOC=DIAG_MTCA:EVR1-ADM:")

# Auto save/restore
save_restoreDebug(2)
dbLoadRecords("db/save_restoreStatus.db", "P=DIAG_MTCA:EVR1-AS:")
save_restoreSet_status_prefix("DIAG_MTCA:EVR1-AS:")

set_savefile_path("${PWD}/as","/save")
set_requestfile_path("${PWD}/as","/req")
system("install -d ${PWD}/as/req")
system("install -d ${PWD}/as/save")

set_pass0_restoreFile("mrf_settings.sav")
set_pass0_restoreFile("mrf_values.sav")
set_pass1_restoreFile("mrf_values.sav")
set_pass1_restoreFile("mrf_waveforms.sav")

# disable use of current time until validation is implemented
var(mrmGTIFEnable, 0)

iocInit()

makeAutosaveFileFromDbInfo("as/req/mrf_settings.req", "autosaveFields_pass0")
makeAutosaveFileFromDbInfo("as/req/mrf_values.req", "autosaveFields")
makeAutosaveFileFromDbInfo("as/req/mrf_waveforms.req", "autosaveFields_pass1")

#create_monitor_set("mrf_settings.req", 5 , "")
#create_monitor_set("mrf_values.req", 5 , "")
#create_monitor_set("mrf_waveforms.req", 30 , "")
