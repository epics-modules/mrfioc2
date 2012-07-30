## Check current time status
generalTimeReport(2)

dbpf("TST{EVG-TrigEvt:0}EvtCode-SP","125")

## Generate a 1Hz trigger if external input is not available
## This assumes that the event clock frequency is 124.9135 MHz
##dbpf("TST{EVG-Mxc:0}Prescaler-SP","124913500")
## Frequency should be 1Hz
##dbpr("TST{EVG-Mxc:0}Frequency-RB",1)

## Select the source of event 125 (Choose from front pannel input 0 or local multiplexed counter Mxc0)
dbpf("TST{EVG-TrigEvt:0}TrigSrc-Sel","Front0")
##dbpf("TST{EVG-TrigEvt:0}TrigSrc-Sel","Mxc0")

dbpf("TST{EVG}1ppsInp-Sel","1")
dbpf("TST{EVG}1ppsInp-MbbiDir_.TPRO","1")

dbpf("TST{EVRRF}Time:Clock-SP","124.913500")
dbpf("TST{PMCEVR}Time:Clock-SP","124.913500")
