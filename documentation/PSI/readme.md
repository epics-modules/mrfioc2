# mrfioc2 in PSI
- [Introduction](#introduction)
- [Problem](#problem)
- [Solution](#solution)
- [Event Receiver - EVR](#event-receiver)
- [Quick start](#quick-start)
    - [Example](#example)
    - [Macro description](#longHeaderName)
- [Advanced](#advanced)

## Introduction
Timing system cards ara available in many form factors. Each of them can have different amount of basic hardware components, such as pulsers, inputs, outputs, SFPs, etc.
To create EPICS database(template) for a specific form factor we would normally use a suitable substitution file and expand it using the MSI tool, which works the same way as the EPICS macro substitutions do. The output of the tool can then be used in our EPICS application.

__However__, in order to set the default values of macros, while recursivly expanding templates (using MSI), we need to use a trick in the original substitution file, which is described in the [advanced section](#substitution-trick).

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
Use a predefined main and supporting EVR templates together with an application substitution file. 
In the application substitution file change/add/remove:

- macros _$(SYS)_ and _$(EVR)_
- optionally apply macros for overriding default values, eg. `$(EVR):PS0-Div-SP=1`
- substitutions/macros for pulser maps, events, event maps, and any other components.
- if needed, add additional soft event triggering

### Example
1. Create a project folder, eg. `MTEST-VME-EVRTEST`
2. Copy the following files to it:
    - mrfioc2/evrMrmApp/Db/PSI/`evr-vmerf230.template` --> `MTEST-VME-EVRTEST/evr-vmerf230.template`
    - mrfioc2/evrMrmApp/Db/PSI/`evrEvent.db` --> `MTEST-VME-EVRTEST/evr-Event.template`
    - mrfioc2/evrMrmApp/Db/PSI/`evrEventmap.db` --> `MTEST-VME-EVRTEST/evr-Eventmap.template`
    - mrfioc2/evrMrmApp/Db/PSI/`evrPulsermap.db` --> `MTEST-VME-EVRTEST/evr-Pulsermap.template`
    - mrfioc2/PSI/`evr_ex.subs` --> `MTEST-VME-EVRTEST/MTEST-VME-EVRTEST_EVR.subs`
    > Please mind the file name and extention changes while copying!
3. Create startup script `MTEST-VME-EVRTEST/MTEST-VME-EVRTEST_startup.script`, so it looks like this:

        ## Load IFC1210 devLib and pev modules
        require 'pev'
        ## Load mrfioc2 device support
        require 'mrfioc2'
        
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
        
        mrmEvrSetupVME(EVR0,2,0x3000000,4,0x28);
        
        ## EVR init done
   
    > Depending on your setup you can also change _mrmEvrSetupVME(EVR0,2,0x3000000,4,0x28);_ to reflect the connected hardware setup.

4. Edit EVR application substitution file `MTEST-VME-EVRTEST/MTEST-VME-EVRTEST_EVR.subs`
    
    The substitution file comprises of main EVR macros and additional event mappings. It should be changed as per the application requirements, and the documentation in the substitution file.
    - search-replace all occurances of _MTEST-VME-BSREAD_ with _MTEST-VME-EVRTEST_
    > Check for the correct EVR name: `EVR0`, and search-replace if needed

    - set the desired values for _Global settings_, _Prescalers_, _Pulsers_, _Front panel inputs_ and _Front panel outputs_ according to the documentation next to the macros for _evr-vmerf230.template_.
    - if needed, add/remove/change the mapping between hardware event code and a software (EPICS) database event in _evr-Event.template_ substitutions
    - if needed, add/remove/change the mapping between hardware event code and a special function of the EVR in _evr-Eventmap.template_ substitutions
    - if needed, add/remove/change the mapping of the hardware event codes to pulse geneators in _evr-Pulsermap.template_ substitutions
    
    > Pulsers can be mapped using three functions to multiple events.

5. Install the prepared IOC (run `swit -V` from your project directory (`MTEST-VME-EVRTEST`))

### Description of the template files in _MTEST-VME-EVRTEST\_EVR.subs_ <a name="longHeaderName"></a>
Example project name is "MTEST-VME-EVRTEST". In the next few chapters we will describe how to use the provided application substitution file, and add/remove/change desired mappings.

The available macros are listed using their name and default value. Where many components with the same macros are available (eg. many pulsers), the macro names will reflect the first component (eg. pulser 0).

> Example: __macroName__=_defaultValue_ : Description, with available settings (`settingName (value)`) or `important information` highlighted.

#### evr-vmerf230.template
1. Main settings
    
    Here we set the system name, the EVR name. and set its global settings. 

    -  __SYS__=_MTEST-VME-BSREAD_ : 
        The system name, eg. MTEST-VME-EVRTEST.
    -  __EVR__=_EVR0_ : 
        The name of the connected Event Receiver, which should be the same as defined using the _mrmEvrSetupVME(...)_ command in startup script.
    -  __EVR0:ExtInhib-Sel__=_0_ :
        Only available in EVRGT! Either honor the hardware inhibit signal(`0`) or don't care about hardware inhibit input state(`1`).
    -  __EVR0:Link-Clk-SP__=_124.916_ :
        The frequency of the EVR's local oscilator. This must be close enough to the EVG master oscilator to allow phase locking with EVR. Available values are `50 MHz - 150 MHz`.
    -  __EVR0:Time-Src-Sel__=_0_ :
        Determines what causes the timestamp event counter to tick.
        - The `event clock(0)` source will use an integer divisor of the EVR's local oscilator. 
        - The `mapped codes(1)` increment the counter whenever certain event arrives. Theese codes can be defined using special mapping records. 
        - `DBus bit 4(2)` will increment the counter on the low-to-high transition of the DBus bit 4.
    -  __EVR0:Time-Clock-SP__=_0.0_ :
        Specifies the rate at which the timestamp event counter will be incremented. This determines the resolution of all timestamps. `This setting is used in conjunction with the 'EVR0:Time-Src-Sel'`.
        
    When the timestamp source is set to "Event clock" this property is used to compute an integer divider from the EVR's local oscilator frequency to the given frequency. Since this may not be exact it is recommended to read back the actual divider setting via the "Timestamp Prescaler" property. In all modes this value is stored in memory and used to convert the timestamp event counter values from ticks to seconds. The units are in `MHz`.

    -  __EVR0:Link-RxMode-Sel__=_1_ :
        Determine wheter only the `DBus(0)` or `DBus + Data Buffer(1)` are sent downstream.

2. Prescalers

    Here we can set the macros for EVRs Prescaler sub-unit. There are three prescalers named PS0, PS1 and PS2.

    - __EVR0:PS0-Div-SP__=_2_ :
        Sets the integer divisor between the Event Clock and the sub-unit output in a range of `2-0xffff`.

3. Pulsers
    
    Here we can set the macros for the EVRs Pulse Generators. There are 16 prescalers available, named Pul0 - Pul15, where __only a subset__ (0-3) of theese __support prescalers__.

    -   __EVR0:Pul0-Ena-Sel__=_1_
        When `disabled(0)`, the output of the pulser will remain in its inactive state (low). The pulser must be `enabled(1)`, before mapped actions will have any effect.
    -   __EVR0:Pul0-Polarity-Sel__=_0_ : 
        Reverses the output polarity. When set, the pulser output changes from `Active High(0)` to `Active Low(1)`.
    -   __EVR0:Pul0-Delay-SP__=_0_ : 
        Determines the time between when the pulser is triggered and when it changes state from inactive to active. Value range: `0 us- 0xffffff us`.
    -   __EVR0:Pul0-Width-SP__=_0_ : 
        Determines the time between when the pulser changes state from inactive to active and when it changes back to inactive. Value range: `0 us- 0xffff us`.
    -   __EVR0:Pul0-Prescaler-SP__=_1_ : 
        Decreases the resolution of both delay and width by an integer multiple. Determines the tick rate of the internal counters used for delay and width, with respect to the ECR's local oscilator. Value range: `0-0xff`.

4. Inputs

    Here we can set the macros for the EVRs Input sub-unit. The number of inputs depends of the hardware model.

    -   __EVR0:FPIn0-Lvl-Sel__=_1_ : 
        When operating in level triggered mode, determines if codes are sent when the input level is low `Active Low(0)` or high `Active High(1)`.
    -   __EVR0:FPIn0-Edge-Sel__=_1_ : 
        When operating in edge triggered mode, determines if codes are sent on the falling `Active Falling(0)` or rising `Active Rising(1)` edge of the input signal.
    -   __EVR0:FPIn0-Trig-Ext-Sel__=_0_ : 
        Selects the condition(`Off(0)`, `Level(1)` or `Edge(2)`) in which to inject event codes into the local mapping ram. These codes are treated as codes comming from the downstream event link.
    -   __EVR0:FPIn0-Code-Ext-SP__=_0_ : 
        Sets the event code which will be applied to the local mapping ram, whenever the 'External mode' condition is met. Event code that triggers this input is in range `0-0xff`.
    -   __EVR0:FPIn0-Trig-Back-Sel__=_0_ : 
        elects the condition(`Off(0)`,  `Level(1)` or `Edge(2)`) in which to send on the upstream event link.
    -   __EVR0:FPIn0-Code-Back-SP__=_0_ : 
        Sets the event code which will be sent on the upstream event link, whenever the 'Backwards mode' condition is met. Event code that triggers this input is in range `0-0xff`.
    -   __EVR0:FPIn0-DBus-Sel__=_0_ : 
        Set the upstream Distributed Bus bit mask which is driven by this input. DBus bits from multiple sources are condensed with a bit-wise OR. Available values are: `0x1 = Bit 0, 0x2 = Bit 1, 0x4 = Bit 2, 0x8 = Bit 3, 0x10 = Bit 4, 0x20 = Bit 5, 0x40 = Bit 6, 0x80 = Bit 7`.

5. Outputs (TTL)

    Here we can set the macros for the EVRs Output sub-unit. Outputs are named either FrontOut#, FrontOutUniv# or RearUniv#, where the range of the number # depends of the hardware model.
    
    Special mapping properties are available to set the outputs:

        | mapping code | output source |
        |--------------|---------------|
        |      0       |  Pulser 0     |
        |      1       |  Pulser 1     |
        |      2       |  Pulser 2     |
        |      3       |  Pulser 3     |
        |      4       |  Pulser 4     |
        |      5       |  Pulser 5     |
        |      6       |  Pulser 6     |
        |      7       |  Pulser 7     |
        |      8       |  Pulser 8     |
        |      9       |  Pulser 9     |
        |      10      |  Pulser 10    |
        |      11      |  Pulser 11    |
        |      12      |  Pulser 12    |
        |      13      |  Pulser 13    |
        |      14      |  Pulser 14    |
        |      15      |  Pulser 15    |
        |      32      |  DBus 0       |
        |      33      |  DBus 1       |
        |      34      |  DBus 2       |
        |      35      |  DBus 3       |
        |      36      |  DBus 4       |
        |      38      |  DBus 6       |
        |      39      |  Dbus 7       |
        |      40      |  Prescaler 0  |
        |      41      |  Prescaler 1  |
        |      42      |  Prescaler 2  |
        |      63      |  Force Low    |
        |      62      |  Force High   |

    -   EVR0:FrontOut0-Ena-SP=1 : 
        When set to `enabled(1)` the mapping property defined in 'EVR0:FrontOut0-Src-SP' is used. When `disabled(0)`, a mapping property of Force Low(63) is used.
    -   EVR0:FrontOut0-Src-SP=63 : 
        `Mapping property codes` from the upper table can be set here. The mapping property code coresponds to an output source.

#### evr-Eventmap.template
There is a number of special functions available, that can trigger on specified event:

- __Blink__ : An LED on the EVRs front panel will blink when the code is received.
- __Forward__ : The received code will be immediately retransmits on the upstream event link.
- __Stop Log__ : Freeze the circular event log buffer. An CPU interrupt will be raised which will cause the buffer to be downloaded. This might be a useful action to map to a fault event.
- __Log__ : Include this event code in the circular event log.
- __Heartbeat__ : This event resets the heartbeat timeout timer.
- __Reset PS__ : Resets the phase of all prescalers.
- __TS reset__ : Transfers the seconds timestamp from the shift register and zeros the sub-seconds part.
- __TS tick__ : When the timestamp source is 'Mapped code' then any event with this mapping will cause the sub-seconds part of the timestamp to increment.
- __Shift 1__ : Shifts the current value of the seconds timestamp shift register up by one bit and sets the low bit to 1.
- __Shift 0__  : Shifts the current value of the seconds timestamp shift register up by one bit and sets the low bit to 0.
- __FIFO__ : Bypass the automatic allocation mechanism and always include this code in the event FIFO.


Using this template we can map a specific hardware event code to a special function of the EVR.

Macros:

- __SYS__ represents the system name,  eg. MTEST-VME-EVRTEST
- __EVR__ represents the name of the connected Event Receiver, which should be the same as defined using the _mrmEvrSetupVME(...)_ command in startup script.
- __EVT__ represents a hardware EVR Event code.
- __FUNC__ represents one of the functions listed above.

Example:
We will blink the EVR0 led on each occurance of event 1, and 6.

    file "evr-Eventmap.template"{
    pattern { SYS,                  EVR,    EVT,   FUNC }
            {"MTEST-VME-EVRTEST",   "EVR0",  "1",   "Blink"}
            {"MTEST-VME-EVRTEST",   "EVR0",  "6",   "Blink"}
    }

#### evr-Pulsermap.template
Event receivers have multiple pulsers that can preform several functions on different events.  It is possible that more than one record will interach with each event code / pulser combinations, however each pairing must be unique. Currently available functions for each pulser are:

- __Trig__ : causes a received event to trigger a pulser.
- __Set__ : causes a received event to force a pulser into active state.
- __Reset__ : causes a received event to force a pulser into inactive state.


Macros:

- __PID__ pulser ID.
- __NAME__ represents a unique mapping name, comprised of the system name, eg. MTEST-VME-EVRTEST, the name of the connected Event Receiver, which should be the same as defined using the _mrmEvrSetupVME(...)_ command in startup script, and unique identifiers for the specific trigger.
- __OBJ__ represents a pulser object comprised of Event Receiver name and Pulser name.
- __F__ represents one of the functions described above.
- __EVT__ represents a hardware EVR Event code.

Example: Pulser 0 of EVR0 will be set to trigger on event 2 and reset on event 3.
        
    file "evr-Pulsermap.template"{
    pattern {PID, NAME,                                                 OBJ,                F,      EVT}
            {0,   "MTEST-VME-EVRTEST-EVR0:DlyGen-$(PID)-Evt-Trig0-SP",  "EVR0:Pul$(PID)",   Trig,    2}
            {0,   "MTEST-VME-EVRTEST-EVR0:DlyGen-$(PID)-Evt-Trig2-SP",  "EVR0:Pul$(PID)",   Reset,   3}
    }


#### evr-Event.template
Provides us with the ability to map between hardware event code and a software (EPICS) database event.

Macros:

- __SYS__ represents the system name, eg. MTEST-VME-EVRTEST.
- __EVR__ represents the name of the connected Event Receiver, which should be the same as defined using the _mrmEvrSetupVME(...)_ command in startup script.
- __CODE__ represents a hardware EVR Event code
- __EVNT__ represents an EPICS database event number (software).

Example:

    file "evr-Event.template"{
    pattern { SYS,                  EVR,    CODE,   EVNT}
            {"MTEST-VME-EVRTEST",   "EVR0", "1",    "1"}
            {"MTEST-VME-EVRTEST",   "EVR0", "2",    "2"}
            {"MTEST-VME-EVRTEST",   "EVR1", "2",    "2"}
    }
We set the system name and reference our Event Receiver using SYS and EVR macros respectively. Then we set and EPIC database event 1 to trigger when a hardware EVR code 1 is received, and event 2 to trigger on HW code 2. It is `recommended to use the same event and code numbers` to avoid confusion during the developmen, but it is not mandatory.

The functionality of EPIC events and links is out of the scope of this documents. It is also possible to manually forward link an appropriate event record, but the details are again out of the scope of this documents.

## Advanced
### Substitution trick
#### Problem
A record with a macro in VAL field might look like this:
`field( VAL , "$(MCR=2)")`
 After using the MSI tool, the field would look like this:
`field( VAL , "2")`
Which means we loose the ability to set the default value in our application. If the macro is defined, we loose the default value.

#### Solution
The record field should be encapsulated using nested macro, like so:
`field( VAL , "$($(OBJ)-Div-SP\=2)")`
Here is what happens:
- `$(OBJ)` is extended when running the MSI tool. This is OK, since it contains template specific information.
- Notice that '=' is escaped. This means that the MSI tool will only unescape the default value, instead of applying it to the undefined macro.
After using the MSI tool with macro `EVR=evr0, OBJ=evr0:PS0` we get
`field( VAL , "$(evr0:PS0-Div-SP=2,undefined)")`
Then we can use macro substitution `evr0:PS0-Div-SP=val` in our application to set the fields value, otherwise we default to '2'.

### Manually generating EVR template
Use a predefined template `evr-vmerf230.template`. If you do not have the template, create one by issuing the following command in `mrfioc2/evrMrmApp/Db/PSI` folder:
    
    msi -S evr-vmerf230.substitutions > evr-vmerf230.template 
    
The command will use the EVR specific _evr-vmerf230.substitutions_ file to output the template to be used in your application.

### Script for settings extraction
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
