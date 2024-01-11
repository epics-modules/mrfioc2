#!../../bin/linux-x86_64/mrf

< envPaths

## Register all support components
dbLoadDatabase("../../dbd/mrf.dbd")
mrf_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("IOCSH_PS1","EVM SeqMask> ")
epicsEnvSet("P","Ether-Realm:EVM")
epicsEnvSet("ENGINEER","agaget")
epicsEnvSet("LOCATION","Etheric Realm")

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","10000000")

epicsEnvSet("EVM_PCI","0000:0e:00.0")
epicsEnvSet("PORT","EVM-MTCA")
epicsEnvSet(FREQ, "88")

mrmEvgSetupPCI("$(PORT)", "$(EVM_PCI)")

## Load record instances
cd $(TOP)
dbLoadRecords("db/evm-mtca-300.uv.db","P=$(P):,s=,EVG=$(PORT),FRF=$(FREQ),FEVT=$(FREQ)")
dbLoadRecords("db/evm-mtca-300-evr.uv.db","P=$(P)U:,EVG=$(PORT),T=U,FRF=$(FREQ),FEVT=$(FREQ)")
dbLoadRecords("db/evm-mtca-300-evr.uv.db","P=$(P)D:,EVG=$(PORT),T=D,FRF=$(FREQ),FEVT=$(FREQ)")

iocInit()

##### Configure the EVM
dbpf $(P):Mxc0Frequency-SP 1
dbpf $(P):Mxc0Prescaler-SP 88000000
## Configure the sequencer
dbpf $(P):SoftSeq0TrigSrc0-Sel "Mxc0"
dbpf $(P):SoftSeq0TsResolution-Sel "uSec"
dbpf $(P):SoftSeq0RunMode-Sel  "Normal"

dbpf $(P):SoftSeq0Timestamp-SP  "[100, 200]"
#Can't dbpf waveform of UCHAR so we use this. Change architecture if needed.
#Event 11 is just used for example
system("$(EPICS_BASE)/bin/linux-x86_64/caput -a $(P):SoftSeq0EvtCode-SP 2 1 11")
system("$(EPICS_BASE)/bin/linux-x86_64/caput -a $(P):SoftSeq0EvtMask-SP 2 1 0")
# Due to this 'enable' settings, event 11 will not b triggered by default
system("$(EPICS_BASE)/bin/linux-x86_64/caput -a $(P):SoftSeq0EvtEna-SP  2 0 3")

# To fully understand mask and enable I suggest you to play with software mask an enable an watch counter of the EVR:
# $(P):SoftSeq0SwMask-Sel
# $(P):SoftSeq0SwEna-Sel
# If you put soft mask to 1 or 3 or 5 etc... it will mask event 1 of the sequence
# because mask of the sequencer is 1, and it's (binary) included in the software mask number
# For enable, soft ena to 3 or 7 or 11.. will enable event 11 of the sequencer
# because ena of the sequencer is 3 and it's (binary) included in the software ena number

## Configure the EVRU
dbpf $(P)U:DlyGen0EvtSet0-SP 13
dbpf $(P)U:DlyGen0EvtReset0-SP 14
dbpf $(P)U:OutFP0SrcPulse-SP "Pulser 0"
# FP0 of the EVRU is the equivalent of Univ8 of the EVM
# So this triggered the Mask 1 when FP0/Univ8 is High.
dbpf $(P):InpUniv8FPMask-Sel "1"


epicsThreadSleep(1)

dbtr $(P):SoftSeq0Commit-Cmd

epicsThreadSleep(1)

dbpf $(P):SoftSeq0Load-Cmd 1
dbpf $(P):SoftSeq0Enable-Cmd 1

# dbpf $(P):TrigEvt1TrigSrc0-Sel  Mxc0
# dbpf $(P):TrigEvt0EvtCode-SP  1

## Push event 13 just to see that upstream/downstream is working
dbpf $(P):TrigEvt1TrigSrc1-Sel  Univ8
dbpf $(P):TrigEvt1EvtCode-SP  13

## 

