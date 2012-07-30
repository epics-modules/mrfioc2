## Configuration EVG
## Select RF input as clock source
dbpf("TST{EVG-EvtClk}Source-Sel","RF")

## Take 500MHz in and divide by 4 to get 125MHz Event clock
##dbpf("TST{EVG-EvtClk}RFFreq-SP","500.0")
##dbpf("TST{EVG-EvtClk}RFDiv-SP","4")

## Take 124.9135 MHz and do not divide to get Event clock
dbpf("TST{EVG-EvtClk}RFFreq-SP","124.9135")
dbpf("TST{EVG-EvtClk}RFDiv-SP","1")

## Event clock should be 124.9135 MHz
dbpr("TST{EVG-EvtClk}Frequency-RB")

## Set clock rate and enable EVRs
## VME-RF EVR
dbpf("TST{EVRRF}Link:Clk-SP","124.9135")
dbpf("TST{EVRRF}Ena-Sel","Enabled")

## PMC EVR
dbpf("TST{PMCEVR}Link:Clk-SP","124.9135")
dbpf("TST{PMCEVR}Ena-Sel","Enabled")
