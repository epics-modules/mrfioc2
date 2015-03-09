## Load IFC1210 devLib and pev modules
require 'pev'
## Load mrfioc2 device support
require 'mrfioc2'
#var pevIntrDebug 255
require 'bufferTest'

epicsEnvSet SYS "PSI-IFC-XXXX"

##########################
#-----! EVG Setup ------!#
##########################
## Configure EVG
## Arguments:
##  - device name
##  - slot number
##  - A24 base address
##  - IRQ level
##  - IRQ vector

mrmEvgSetupVME(EVG0,2,0x000000000,0x1,0x1);

## EVG Databases:

## Load EVG record instances
#dbLoadRecords("evg-vme.db","SYS=$(SYS),EVG=EVG0")

## Load 2 EVG soft sequences
dbLoadRecords("evgSoftSeq.db","SYS=$(SYS),EVG=EVG0,SEQNUM=1,NELM=2047")
dbLoadRecords("evgSoftSeq.db","SYS=$(SYS),EVG=EVG0,SEQNUM=2,NELM=2047")


##########################
#-----! EVR Setup ------!#
##########################
## Configure EVG
## Arguments:
##  - device name
##  - slot number
##  - A24 base address
##  - IRQ level
##  - IRQ vector

mrmEvrSetupVME(EVR0,3,0x3000000,4,0x28);

dbLoadRecords("evr-vmerf230.db","SYS=$(SYS),EVR=EVR0")

dbLoadRecords("evrEvent.db","SYS=$(SYS),EVR=EVR0,CODE=1,EVNT=1")
dbLoadRecords("evrEvent.db","SYS=$(SYS),EVR=EVR0,CODE=2,EVNT=2")
dbLoadRecords("evrEvent.db","SYS=$(SYS),EVR=EVR0,CODE=3,EVNT=3")

## Init done
iocInit

## Set EVR0 as the receiving end
bufferMonitor("EVR0")

## Send sample data from EVG0
bufferSend("EVG0")

