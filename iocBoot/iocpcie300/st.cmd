
#< envPaths

dbLoadDatabase("dbd/mrf.dbd")
mrf_registerRecordDeviceDriver(pdbbase)

epicsEnvSet SYS PC-EVR300E-TEST
epicsEnvSet EVR EVR0
epicsEnvSet TEMPLATE_DIR "../../db"

mrmEvrSetupPCI($(EVR),1,2,0)


dbLoadTemplate "evr_ex_PCIe-300.subs" EVR_=$(EVR),SYS_=$(SYS),SYS=$(SYS),EVR=$(EVR)"

iocInit()

