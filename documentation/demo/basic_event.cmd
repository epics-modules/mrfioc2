## Generate a basic Event and decode it in the receivers
## Setting clocks and enabling EVG and EVRs is needed (see e.g. demo_clk_fracsync.cmd)

## Make EVG send a periodic event (1Hz rate)
dbpf("TST{EVG-Mxc:0}Prescaler-SP","12491350")
## Frequency should be 1Hz
dbpr("TST{EVG-Mxc:0}Frequency-RB",1)

dbpf("TST{EVG-TrigEvt:1}EvtCode-SP","122")
dbpf("TST{EVG-TrigEvt:1}TrigSrc-Sel","Mxc0")

##dbpf("TST{EVG-TrigEvt:1}EvtCode-SP","125")
##dbpf("TST{EVG-TrigEvt:1}TrigSrc-Sel","Mxc0")

## Map events to EVR outputs
## VME-RF EVR
dbpf("TST{EVRRF-DlyGen:0}Ena-Sel","1")
dbpf("TST{PMCEVR-DlyGen:0}Ena-Sel","1")

dbpf("TST{EVRRF-DlyGen:0}Evt:Trig0-SP", "122")
dbpf("TST{PMCEVR-DlyGen:0}Evt:Trig0-SP", "122")

## Play with widths
dbpf("TST{EVRRF-DlyGen:0}Width-SP","1")
dbpf("TST{PMCEVR-DlyGen:0}Width-SP","1")

dbpf("TST{EVRRF-Out:FP0}Src:Pulse-SP", "0")
dbpf("TST{PMCEVR-Out:FP0}Src:Pulse-SP", "0")

## Play with delays
##dbpf("TST{EVRRF-DlyGen:0}Delay-SP","2")
dbpf("TST{PMCEVR-DlyGen:0}Delay-SP","2")
