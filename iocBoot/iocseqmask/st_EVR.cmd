#!../../bin/linux-x86_64/mrf

< envPaths

## Register all support components
dbLoadDatabase("../../dbd/mrf.dbd")
mrf_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("IOCSH_PS1","EVR SeqMask> ")
epicsEnvSet("P","Ether-Realm:EVR")
epicsEnvSet("ENGINEER","agaget")
epicsEnvSet("LOCATION","Etheric Realm")

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","10000000")

epicsEnvSet("EVR_PCI","0000:0c:00.0")


mrmEvrSetupPCI("EVR-MTCA", "$(EVR_PCI)")

cd $(TOP)
## Load record instances
dbLoadRecords("db/evr-mtca-300u.uv.db","P=$(P):,EVR=EVR-MTCA,PNDELAY=PNDELAY,PNWIDTH=PNWIDTH,FRF=88,FEVT=88")


iocInit()

## we use Backplane of the MTCA card here to upstream event through input, like that no need for any other phyical installation.
## Events 2 and 3 will be use to mask and unmask the event 1 in the sequencer. You can send them softwarely through the EVM.
## Indeed You can remove that part and use analogic signal in other inputs if you prefer.
### When we receive event 2 Pulser 1-> backplane 1 is triggered
dbpf $(P):DlyGen1Width-SP 100
dbpf $(P):DlyGen1Width-RB
dbpf $(P):DlyGen1EvtTrig0-SP 2
dbpf $(P):OutBack1SrcPulse-SP "Pulser 1"

#When we receive event 3 Pulser 2-> backplane 2 is triggered
dbpf $(P):DlyGen2Width-SP 100
dbpf $(P):DlyGen2Width-RB
dbpf $(P):DlyGen2EvtTrig0-SP 3
dbpf $(P):OutBack2SrcPulse-SP "Pulser 2"

###Backplane channel 1 use for masking
dbpf $(P):BPIn1TrigBack-Sel "Edge"
dbpf $(P):BPIn1CodeBack-SP 13

###Backplane channel 2 use for unmasking
dbpf $(P):BPIn2TrigBack-Sel "Edge"
dbpf $(P):BPIn2CodeBack-SP 14

## Monitoring events. When your sequencer is running normally, counter A should increment at 1Hz.
## When event 2 is send to this EVR, counterA will stop incrementing, counterB will increment + 1, and counterD will increment +1
## When event 3 is send to this EVR, counterA will restart incrementing, counterC will increment + 1
dbpf $(P):EvtA-SP.OUT "@OBJ=EVR-MTCA,Code=1"
dbpf $(P):EvtB-SP.OUT "@OBJ=EVR-MTCA,Code=2"
dbpf $(P):EvtC-SP.OUT "@OBJ=EVR-MTCA,Code=3"
dbpf $(P):EvtD-SP.OUT "@OBJ=EVR-MTCA,Code=13"
dbpf $(P):EvtE-SP.OUT "@OBJ=EVR-MTCA,Code=11"

##

