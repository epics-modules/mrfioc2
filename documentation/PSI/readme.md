# mrfioc2 in PSI
- [Introduction](#introduction)
- [Problem](#problem)
- [Solution](#solution)
- [Event Receiver - EVR](#event-receiver)
- [Quick start](#quick-start)
    - [Example](#example)
- [Script for settings extraction](#script-for-settings-extraction)

## Introduction
Timing system cards ara available in many form factors. Each of them can have different amount of basic hardware components, such as pulsers, inputs, outputs, SFPs, etc.
To create EPICS database(template) for a specific form factor we would normally use a suitable substitution file and expand it using the MSI tool, which works the same way as the EPICS macro substitutions do. The output of the tool can then be used in our EPICS application.

__However__, in order to set the default values of macros, while recursivly expanding templates (using MSI), we need to use a trick in the original substitution file.

## Problem
A record with a macro in VAL field might look like this:
`field( VAL , "$(MCR=2)")`
 After using the MSI tool, the field would look like this:
`field( VAL , "2")`
Which means we loose the ability to set the default value in our application. If the macro is defined, we loose the default value.

## Solution
The record field should be encapsulated using nested macro, like so:
`field( VAL , "$($(OBJ)-Div-SP\=2)")`
Here is what happens:
- `$(OBJ)` is extended when running the MSI tool. This is OK, since it contains template specific information.
- Notice that '=' is escaped. This means that the MSI tool will only unescape the default value, instead of applying it to the undefined macro.
After using the MSI tool with macro `EVR=evr0, OBJ=evr0:PS0` we get
`field( VAL , "$(evr0:PS0-Div-SP=2,undefined)")`
Then we can use macro substitution `evr0:PS0-Div-SP=val` in our application to set the fields value, otherwise we default to '2'.

##Event Receiver
Each Event Receiver (EVR) consists of a number of basic _components_ and a _mapping ram_. Components are as follows:
- basic settings (event link speed, clock and link status, time source and timestamp tick rate select, downstream mode)
- input settings (level, edge select, backwards or external mode selection, DBus mask settings)
- output settings (enable/disable, output routing of pulsers, DBus, prescalers or low/high signal)
- prescaler settings (Integer divisor selection and output frequency display)
- pulser settings (enable/disable, polarity, pulse delay, pulse width and prescaler settings)

All of theese are available through a predefined template `evr-vmerf230.template`, and can be configured through a substitution file or at runtime. _Mapping ram_ is used to associate components to events in order to achieve the desired functionality of the EVR. There exists two types of mapping:
- mapping between hardware event code and a software database event
    - here, we can specify soft events that can be used in EPICS records to trigger processing based on the event code received
- mapping between hardware event code and EVR component
    -  mapping a special EVR function (_"FIFO", "Latch TS", "Blink", "Forward","Stop Log", "Log", "Heartbeat", "Reset PS", "TS reset", "TS tick", "Shift 1", "Shift 0"_) to an event code
    -  mapping a pulser function (_"Trig", "Set", "Reset"_) to an event code

An [example](#example) of the described functionalities is available [below](#example).

## Quick start
Use a predefined template `evr-vmerf230.template`. If you do not have the template, create one by issuing the following command in `mrfioc2/evrMrmApp/Db/PSI` folder:
    
    msi -S evr-vmerf230.substitutions > evr-vmerf230.template 
    
Now you need to set up your application substitution file:
- apply macros _$(SYS)_ and _$(EVR)_
- optionally apply macros for overriding default values, eg. `$(EVR):PS0-Div-SP=1`
- add substitutions/macros for pulser maps, events, event maps, and any other components.

### Example
Copy the following files to your project folder:
- mrfioc2/evrMrmApp/Db/PSI/`evr-vmerf230.template`
- mrfioc2/evrMrmApp/Db/PSI/`evrEvent.db`
- mrfioc2/evrMrmApp/Db/PSI/`evrEventmap.db`
- mrfioc2/evrMrmApp/Db/PSI/`evrPulsermap.db`

Example project name is "MTEST". Create your main EVR substitution file, and add desired components:
- let's set up two Event Receivers EVR0 and EVR1. 
        file "evr-vmerf230.template"{
    	    {SYS="MTEST", EVR="EVR0"},
    	    {SYS="MTEST", EVR="EVR1", EVR1:PS1-Div-SP=4},
        }
    The first substitution will create records for event receiver number 0 and set them to default values, where applicable.
    
    The second substitution will create records for event receiver number 1, set the value of prescaler 1 to 4 and set the other records to their default values, where applicable.
- We can add mappings between hardware event code and a software database event
        file "evrEvent.db"{
        pattern { SYS,      EVR,    CODE,   EVNT}
            	{"MTEST",   "EVR0", "1",    "1"}
            	{"MTEST",   "EVR0", "2",    "2"}
            	{"MTEST",   "EVR1", "2",    "2"}
        }
- There is a number of special functions available (_"FIFO", "Latch TS", "Blink", "Forward","Stop Log", "Log", "Heartbeat", "Reset PS", "TS reset", "TS tick", "Shift 1", "Shift 0"_), that can trigger on specified event. We will blink the EVR0 led on each occurance of event 1, and EVR1 led on each occurance of event 2.
        file "evrEventmap.db"{
        pattern { SYS,      EVR,    EVT,   FUNC }
                {"MTEST",   "EVR0",  "1",   "Blink"}
                {"MTEST",   "EVR1",  "2",   "Blink"}
        }
- Event receivers have multiple pulsers that can emit predefined pulses on different events. Pulser 0 of EVR0 will be set to trigger on event 2.
        file "evrPulsermap.db"{
        pattern {PID, NAME,                                     OBJ,                F,      EVT}
                {0,   "MTEST-EVR0:DlyGen-$(PID)-Evt-Trig0-SP",  "EVR0:Pul$(PID)",   Trig,   2}
        }

## Script for settings extraction
In order to see exactly which macros for the specific EVR form factor template can be overriden in your application, use the settings extraction script. It is located in `mrfioc2/utils/extractSettings.sh`. When run from this location, it will use the folder `mrfioc2/evrMrmApp/Db/PSI` and the substitution file `evr-vmerf230.substitutions` in this folder to output the settings into `mrfioc2/utils/settings.txt`.

__Note__ that the script also expands macros _SYS=VME, EVR=EVRx_ for better readability.

An example of `settings.txt` file:

    EVRx:Ena-Sel=0
    EVRx:ExtInhib-Sel=0
    EVRx:Link-Clk-SP=124.916
    EVRx:Time-Src-Sel=0
    EVRx:Time-Clock-SP=0.0
    EVRx:Link-RxMode-Sel=1
    EVRx:PS0-Div-SP=2
    EVRx:PS1-Div-SP=2
    EVRx:PS2-Div-SP=2
    EVRx:Pul0-Ena-Sel=1
    EVRx:Pul0-Polarity-Sel=0
    EVRx:Pul0-Delay-SP=0
    EVRx:Pul0-Width-SP=0
    EVRx:Pul0-Prescaler-SP=1
    EVRx:Pul1-Ena-Sel=1
    EVRx:Pul1-Polarity-Sel=0
    EVRx:Pul1-Delay-SP=0
    EVRx:Pul1-Width-SP=0
    EVRx:Pul1-Prescaler-SP=1
    EVRx:Pul2-Ena-Sel=1
    EVRx:Pul2-Polarity-Sel=0
    EVRx:Pul2-Delay-SP=0
    EVRx:Pul2-Width-SP=0
    EVRx:Pul2-Prescaler-SP=1
    EVRx:Pul3-Ena-Sel=1
    EVRx:Pul3-Polarity-Sel=0
    EVRx:Pul3-Delay-SP=0
    EVRx:Pul3-Width-SP=0
    EVRx:Pul3-Prescaler-SP=1
    EVRx:Pul4-Ena-Sel=1
    EVRx:Pul4-Polarity-Sel=0
    EVRx:Pul4-Delay-SP=0
    EVRx:Pul4-Width-SP=0
    EVRx:Pul4-Prescaler-SP=1
    EVRx:Pul5-Ena-Sel=1
    EVRx:Pul5-Polarity-Sel=0
    EVRx:Pul5-Delay-SP=0
    EVRx:Pul5-Width-SP=0
    EVRx:Pul5-Prescaler-SP=1
    EVRx:Pul6-Ena-Sel=1
    EVRx:Pul6-Polarity-Sel=0
    EVRx:Pul6-Delay-SP=0
    EVRx:Pul6-Width-SP=0
    EVRx:Pul6-Prescaler-SP=1
    EVRx:Pul7-Ena-Sel=1
    EVRx:Pul7-Polarity-Sel=0
    EVRx:Pul7-Delay-SP=0
    EVRx:Pul7-Width-SP=0
    EVRx:Pul7-Prescaler-SP=1
    EVRx:Pul8-Ena-Sel=1
    EVRx:Pul8-Polarity-Sel=0
    EVRx:Pul8-Delay-SP=0
    EVRx:Pul8-Width-SP=0
    EVRx:Pul8-Prescaler-SP=1
    EVRx:Pul9-Ena-Sel=1
    EVRx:Pul9-Polarity-Sel=0
    EVRx:Pul9-Delay-SP=0
    EVRx:Pul9-Width-SP=0
    EVRx:Pul9-Prescaler-SP=1
    EVRx:Pul10-Ena-Sel=1
    EVRx:Pul10-Polarity-Sel=0
    EVRx:Pul10-Delay-SP=0
    EVRx:Pul10-Width-SP=0
    EVRx:Pul10-Prescaler-SP=1
    EVRx:Pul11-Ena-Sel=1
    EVRx:Pul11-Polarity-Sel=0
    EVRx:Pul11-Delay-SP=0
    EVRx:Pul11-Width-SP=0
    EVRx:Pul11-Prescaler-SP=1
    EVRx:Pul12-Ena-Sel=1
    EVRx:Pul12-Polarity-Sel=0
    EVRx:Pul12-Delay-SP=0
    EVRx:Pul12-Width-SP=0
    EVRx:Pul12-Prescaler-SP=1
    EVRx:Pul13-Ena-Sel=1
    EVRx:Pul13-Polarity-Sel=0
    EVRx:Pul13-Delay-SP=0
    EVRx:Pul13-Width-SP=0
    EVRx:Pul13-Prescaler-SP=1
    EVRx:Pul14-Ena-Sel=1
    EVRx:Pul14-Polarity-Sel=0
    EVRx:Pul14-Delay-SP=0
    EVRx:Pul14-Width-SP=0
    EVRx:Pul14-Prescaler-SP=1
    EVRx:Pul15-Ena-Sel=1
    EVRx:Pul15-Polarity-Sel=0
    EVRx:Pul15-Delay-SP=0
    EVRx:Pul15-Width-SP=0
    EVRx:Pul15-Prescaler-SP=1
    EVRx:FPIn0-Lvl-Sel=1
    EVRx:FPIn0-Edge-Sel=1
    EVRx:FPIn0-Trig-Ext-Sel=0
    EVRx:FPIn0-Code-Ext-SP=0
    EVRx:FPIn0-Trig-Back-Sel=0
    EVRx:FPIn0-Code-Back-SP=0
    EVRx:FPIn0-DBus-Sel=0
    EVRx:FPIn1-Lvl-Sel=1
    EVRx:FPIn1-Edge-Sel=1
    EVRx:FPIn1-Trig-Ext-Sel=0
    EVRx:FPIn1-Code-Ext-SP=0
    EVRx:FPIn1-Trig-Back-Sel=0
    EVRx:FPIn1-Code-Back-SP=0
    EVRx:FPIn1-DBus-Sel=0
    EVRx:FrontOut0-Ena-SP=1
    EVRx:FrontOut0-Src-SP=63
    EVRx:FrontOut1-Ena-SP=1
    EVRx:FrontOut1-Src-SP=63
    EVRx:FrontOut2-Ena-SP=1
    EVRx:FrontOut2-Src-SP=63
    EVRx:FrontOut3-Ena-SP=1
    EVRx:FrontOut3-Src-SP=63
    EVRx:FrontOut4-Ena-SP=1
    EVRx:FrontOut4-Src-SP=63
    EVRx:FrontOut5-Ena-SP=1
    EVRx:FrontOut5-Src-SP=63
    EVRx:FrontOut6-Ena-SP=1
    EVRx:FrontOut6-Src-SP=63
    EVRx:FrontOut7-Ena-SP=1
    EVRx:FrontOut7-Src-SP=63
    EVRx:FrontUnivOut0-Ena-SP=1
    EVRx:FrontUnivOut0-Src-SP=63
    EVRx:FrontUnivOut1-Ena-SP=1
    EVRx:FrontUnivOut1-Src-SP=63
    EVRx:FrontUnivOut2-Ena-SP=1
    EVRx:FrontUnivOut2-Src-SP=63
    EVRx:FrontUnivOut3-Ena-SP=1
    EVRx:FrontUnivOut3-Src-SP=63
