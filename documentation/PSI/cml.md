# CML

Front Panel CML Outputs provide low jitter differential signals with special outputs. The outputs can work in different configurations waveform (pulse) mode, pattern mode and frequency mode.
CML outputs correspond to Front Pannel ouputs 4-6. If CML is disabled, normal TTL outputs are used.

## Pattern mode
There are 4 configurable waveforms (20 bit pattern), each corresponds to a state of the signal:

- signal is __rising__: _Pat:Rising-SP_ waveform is sent out after the rising edge is detected and will interrupt the current pattern being sent.

- signal is __falling__: _Pat:Falling-SP_ waveform is sent out after the falling edge is detected and will interrupt the current pattern being sent.

- signal is __high__: _Pat:High-SP_ waveform is sent out after the edge is detected and will repeat until the next edge - when the signal is stable high.

- signal is __low__: _Pat:Low-SP_ waveform is sent out after the edge is detected and will repeat until the next edge - when the signal is stable low.

The waveforms are configured using corresponding _mbboDirect_ records, eg. Pat:Low0_15-SP can be used to configure the 0-15 MSB of the _Pat:Low-SP_ waveform.

## Frequency mode
In the frequency mode we can generate square signals when the configured trigger occurs. The time for the frequency generator can be defined in steps of 1/20 part of the event clock cycle.

- Trigger __Level__ (_Freq:Lvl-SP_): when a trigger arrives the output is forced to this level.

- trigger __point__ (_Freq:Init-SP_): set the starting point of the square signal. This allows for a phase difference between the trigger source and the ouput.

- time __active__ (_Freq:High-SP_): sets the amount of time the signal is in active state

- time __inactive__ (_Freq:Low-SP_): sets the amount of time the signal is in inactive state

## Waveform mode
In waveform mode, the user can send out arbitrary waveforms. A waveform can be a multiple of 20 in length, with maximum length 20x2048 bits.

- __cycle mode__ (_Pat:WfCycle-SP_): In single shot mode the waveform in sent only once per received trigger, where in loop mode the pattern will continously loop after the first trigger occured.

- __pattern__ (_Pat:Wf-SP_): the pattern to be sent out

- __timeline__ (_Pat:WfX-I_): displays the times at which an element from the pattern will be sent out.

A waveform delay calculator can be used to automatically generate waveforms:

- __enable__ (_WfCalc:Ena-SP_): can be used to enable or disable pattern calculation.

- __delay__ (_WfCalc:Delay-SP_): can be used to specify the time of the low periods in the pattern

- __width__ (_WfCalc:Width-SP_): can be used to specify the time of the high periods in the pattern

> note that the last bit of the pattern is allways 0!

Special waveform generator for bunches is also available. 

- __enable__ (_BunchTrain:Ena-SP_): enable or disable bunch pattern calculator

- __size__ (_BunchTrain:Size-SP_): set the number of bunches per train. One bunch pattern consists of 5 high bits and 5 low bits. It looks like this: -----_ _ _ _ _. A train allways starts with 10 low bits. The train with three bunches looks like this: _ _ _ _ _ _ _ _ _ _-----_ _ _ _ _-----_ _ _ _ _.