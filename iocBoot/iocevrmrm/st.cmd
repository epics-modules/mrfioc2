
#< envPaths

cd("../..")

## Register all support components
dbLoadDatabase("dbd/mrf.dbd")
mrf_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("ENGINEER","mdavidsaver x3698")
epicsEnvSet("LOCATION","Blg 902 Rm 28")

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","1000000")

mrmEvrSetupPCI("PMCEVR",0,5,0)

dbLoadRecords("db/evr-pmc-230.db","SYS=TST, D=evr:1, EVR=PMCEVR")

## EVR Pulser Alias
#dbLoadRecords("db/evralias.db", "PID=0,SYS=TST, D=evr:2,PN=$(SYS){$(D)-DlyGen:$(PID)},PNA=$(SYS){$(D)-Alias:$(PID)}")

mrmEvgSetupPCI("EVG", 0, 6, 0)

dbLoadRecords("db/vme-evg230.db", "SYS=TST, D=evg:1, EVG=EVG")

#dbLoadRecords("db/iocAdminRTEMS.db", "IOC=mrftest")

# Auto save/restore
#save_restoreDebug(2)
#dbLoadRecords("db/save_restoreStatus.db", "P=mrftest:")
#save_restoreSet_status_prefix("mrftest:")

#set_savefile_path("${mnt}/as","/save")
#set_requestfile_path("${mnt}/as","/req")

#set_pass0_restoreFile("mrf_settings.sav")
#set_pass0_restoreFile("mrf_values.sav")
#set_pass1_restoreFile("mrf_values.sav")
#set_pass1_restoreFile("mrf_waveforms.sav")

iocInit()

#makeAutosaveFileFromDbInfo("as/req/mrf_settings.req", "autosaveFields_pass0")
#makeAutosaveFileFromDbInfo("as/req/mrf_values.req", "autosaveFields")
#makeAutosaveFileFromDbInfo("as/req/mrf_waveforms.req", "autosaveFields_pass1")

#create_monitor_set("mrf_settings.req", 5 , "")
#create_monitor_set("mrf_values.req", 5 , "")
#create_monitor_set("mrf_waveforms.req", 30 , "")

mrmEvgSoftTime("EVG")
