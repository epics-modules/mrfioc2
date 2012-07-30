## Configuration EVG
dbpf("TST{EVG-EvtClk}Source-Sel","FracSyn")
dbpf("TST{EVG-EvtClk}FracSynFreq-SP","125.0")

## Event clock should be 125MHz
dbpr("TST{EVG-EvtClk}Frequency-RB")

## Set clock rate and enable EVRs
## VME-RF EVR
dbpf("TST{EVRRF}Link:Clk-SP","125.0")
dbpf("TST{EVRRF}Ena-Sel","Enabled")

## PMC EVR
dbpf("TST{PMCEVR}Link:Clk-SP","125.0")
dbpf("TST{PMCEVR}Ena-Sel","Enabled")
