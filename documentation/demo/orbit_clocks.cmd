## Setup orbit clocks

## COIC (500MHz/(328*125)=125MHz/(328*125/4)=125MHz/10250=12.195kHz)
## Generate COIC and wire it to DBus[7]
dbpf("TST{EVG-Mxc:7}Prescaler-SP","10250")
dbpr("TST{EVG-Mxc:7}Frequency-RB",1)
dbpf("TST{EVG-Dbus:7}Src-Sel",7)

## Pull out COIC on FP0 (EVR-RF)
dbpf("TST{EVRRF-Out:FP0}Src:DBus-SP",7)

## BROC (125 RF buckets, 250ns period)
dbpf("TST{EVRRF-Out:FP4}Mode-Sel","Frequency")
dbpf("TST{EVRRF-Out:FP4}Freq:High-SP", "312")   ## (ns)
dbpf("TST{EVRRF-Out:FP4}Freq:Low-SP", "313")    ## (ns)
dbpf("TST{EVRRF-Out:FP4}Freq:Lvl-SP","0")     ## Force level on COIC arrival
dbpf("TST{EVRRF-Out:FP4}Ena-Sel","1")
dbpf("TST{EVRRF-Out:FP4}Pwr-Sel","On")
#dbpf("TST{EVRRF-Out:FP4}Freq:Init-SP","0")

## SROC (328 RF buckets, 656ns period)
dbpf("TST{EVRRF-Out:FP5}Mode-Sel","Frequency")
dbpf("TST{EVRRF-Out:FP5}Freq:High-SP", "820")   ## (ns)
dbpf("TST{EVRRF-Out:FP5}Freq:Low-SP", "820")    ## (ns)
dbpf("TST{EVRRF-Out:FP5}Freq:Lvl-SP","0")     ## Force level on COIC arrival
dbpf("TST{EVRRF-Out:FP5}Ena-Sel","1")
dbpf("TST{EVRRF-Out:FP5}Pwr-Sel","On")
#dbpf("TST{EVRRF-Out:FP5}Freq:Init-SP","0")

## Use as reset trigger for BROC and SROC
dbpf("TST{EVRRF-Out:FP4}Src:DBus-SP",7)
dbpf("TST{EVRRF-Out:FP5}Src:DBus-SP",7)
