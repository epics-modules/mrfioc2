
---------------Epics device driver for MRF VME-EVG-230-------------------------

Author: Jayesh Shah, NSLS2, BNL
		jshah@bnl.gov

Acknowlegement:
	Parts of the MRF VME-EVG-230 device driver code has been reused from 
		- Epics device driver for MRF EVR-230 by Micheal Davidsaver, NSLS2, BNL.
		- Epics device driver for MRF EVG-230 by Eric Bjorklund, LANSCE, LANL.


Read evgTODO.txt to get the current status of the driver development.

TODO: Introduction to Timing System
TODO: Introduction to EVG
TODO: Introduction to EVR 

Brief note about all the classes/Sub-Component of VME-EVG-230 driver
-------------------------------------------------------------------------------
EVG:
-------------------------------------------------------------------------------
	Usage:
		caput EVG$(cardNum):Enable <Enable/Disable>
	Enable or Disable the EVG.

	Macro: 	$(cardNum) = Logical card number EVG

	Class: evgMrm
	Files: evgMrm.h/evgMrm.cpp/devEvgMrm.cpp/evgMrm.db

-------------------------------------------------------------------------------
Event Clock:
-------------------------------------------------------------------------------
	All the operations on EVG are synchronised to the event clock. Which is derived
	from either externally provided RF clock or from an on-board fractional synthesiser.

	 Usage:	 
		caput EVG$(cardNum):ClkSpeed <clkspeed>
	Set the frequency or speed of the Event Clock in MHz. <clkspeed> can range from 50 to 125.
	This pv is used to set the clk speed if the clk source is internal(i.e. clksrc == 0).
	If the clk source is external(i.e clksrc > 0) then the requested clk speed is recorded 
	and is used whenever the Clk Source changes to internal.

		caput EVG$(cardNum):RFref <RFfreq>
	Set the RF Input frequency. <RFfreq> can range from 50MHz to 1.6GHz.
	It is required to set this before selecting the Event clock source to be
 	external (i.e. clksrc > 0). 

		caput EVG$(cardNum):ClkSrc <clksrc>
	Set the Source of Event Clock. <clksrc> can range from 0 to 32.
	If clksrc == 0 means clk source is internal i.e on-board fractional synthesiser.
 	The clk speed will be set to the previously requested clkspeed. 
	If clksrc > 0 means clk source is external and the clk speed will be set to
 	(RF frequency /clksrc). So to have clk speed of RF/4 command will be
	caput EVG$(cardNum):ClkSrc 4

	Macro: 	$(cardNum) = Logical card number EVG
	
	Class: evgEvtClk
	Files: evgEvtClk.h/evgEvtClk.cpp/devEvgEvtClk.cpp/evgEvtClk.db

-------------------------------------------------------------------------------
Software Events:
-------------------------------------------------------------------------------
	Software event is used to send out an event code by writing that event to a
	particular register in EVG.

	Usage:
		caput EVG$(cardNum):SoftEvtEna <Enable/Disable>
	Enable/Disable the soft event transmission.

		caput EVG$(cardNum):SoftEvtCode <evtCode>
	Sends out the Event Code <evtCode> to the event stream. <evtCode> can range form 0 to 255.

	Macro: 	$(cardNum) = Logical card number EVG
	
	Class: evgSoftEvt
	Files: evgSoftEvt.h/evgSoftEvt.cpp/devEvgSoftEvt.cpp/evgSoftEvt.db

-------------------------------------------------------------------------------
Trigger Events:
-------------------------------------------------------------------------------
	There are currently 8 trigger event sources. Trigger events are used to send
	out event code into the event streams. The <evtCode> to be transmitted is determined 
	by the contents of the event code register of Triger Event. The stimulus may
 	be a detected rising edge on an external signal or a rising edge of a 
	multliplexed counter output.

	Usage:
		caput EVG$(cardNum):TrigEvt$(trigEvtNum)Ena <Enable/Disable>
	Enable or Disable the transmission of Trigger Events.

		caput EVG$(cardNum):TrigEvt$(trigEvtNum)Code <evtCode>
	Sets the Event Code <evtCode> to be sent out, whenever it detects a rising
 	edge on an external signal or a rising edge of a multliplexed counter output.
	<evtCode> can range form 0 to 255.

	Macro: 	$(cardNum) = Logical card number EVG
			$(trigEvtNum) = The trigger event ID(0 to 7)

	Class: evgTrigEvt
	Files: evgTrigEvt.h/evgTrigEvt.cpp/devEvgTrigEvt.cpp/evgTrigEvt.db

-------------------------------------------------------------------------------
Distributed bus:
-------------------------------------------------------------------------------
	The bits of the distributed bus can be driven by any of the three sources
	1. Sampling the external input signals at event rate
	2. Forwarding the output of corresponding multiplexed counter
	3. Forwarding the state of all distributed bus bits of upstram EVG.
	There are in all 8 distributed bus bits.

	Usage:
		caput EVG$(cardNum):Dbus$(dbusBit)Map <Off/External/Mxc/UpstreamEVG>
		
	Macro: 	$(cardNum) = Logical card number EVG
			$(dbusBit) = The distributed bus bit(0 to 7)

	Class: evgDbus
	Files: evgDbus.h/evgDbus.cpp/devEvgDbus.cpp/evgDbus.db

-------------------------------------------------------------------------------
Multiplexed Counter:
-------------------------------------------------------------------------------
	There are 8 32-bit multiplexed counters that generate clock signals with 
	programmable frequencies from event clock/2^32-1 to event clock/2. 
	The counter outputs may be programmed to trigger events, drive distributed
	bus signals and trigger sequence RAMs.  

	Usage:
		caput EVG$(cardNum):Mxc$(mxcNum):Polarity <Original/Inverted>
	Set the Multiplex Counter(Mxc) output polarity.

		caput EVG$(cardNum):Mxc$(mxcNum):Presacler <prescaler>
	The <prescaler> determines the output frequency of the Mxc.
	For output frequency of (event clock/2) the <prescaler> should be 2.
		
		caput EVG$(cardNum):Mxc$(mxcNum):Freq <freq>
	It is used to set the output frequency of the Mxc in Hz.

		caput EVG$(cardNum):Mxc$(mxcNum)TrigEvtMap.Bx = <1/0>
	Map rising edge of Mxc to send out trigger event 'x'. Where 'x' could range from 0 to 7

	Macro: 	$(cardNum) = Logical card number EVG
			$(mxcNum) = ID for multiplexed counter(0 to 7)

	Class: evgMxc
	Files: evgMxc.h/evgMxc.cpp/devEvgMxc.cpp/evgMxc.db

-------------------------------------------------------------------------------
Input:
-------------------------------------------------------------------------------
	It is used to configure the 2 front panel input and 4 front panel universal
 	inputs.

	Usage:
		caput EVG$(cardNum):$(inpType)$(inpNum):DbusMap.Bx 1
	Map the front panel input to the distribured bus bit 'x'. Where 'x' could
 	range from 0 to 7.
	
		caput EVG$(cardNum):$(inpType)$(inpNum):TrigEvtMap.Bx 1
	Map the front panel input to server as trigger to the Trigger Event 'x'.
 	Where 'x' could range from 0 to 7.

		caput EVG$(cardNum):$(inpType)$(inpNum):EnaIrq <Enable/Disable>
	Enable or Disable the External Interrupt. When enabled, an interrupt is
 	received on every rising edge the input signal.

	Macro: 	$(cardNum) = Logical card number EVG
			$(inpType) = FP_Input or Univ_Input
			$(inpNum) = The front panel Input ID
	
	Class: evgInput
	Files: evgInput.h/evgInput.cpp/devEvgInput.cpp/evgInput.db

-------------------------------------------------------------------------------
Output:
-------------------------------------------------------------------------------
	It is used to configure the 4 front panel outputs and 4 four front panel
 	universal outputs.

	Usage:
		caput EVG$(cardNum):$(outType)$(outNum):Map <map>
	Where: map = "Dbus0 - Dbus7" to map the disributed bus bit 0 through 7
 				  to the output
			   = "Logic High" to force logic 1 from ouput
			   = "Logic Low" to force logic 0 from ouput
	
	Macro: 	$(cardNum) = Logical card number EVG
			$(outType) = FP_Output or Univ_Output
			$(outNum) = The front panel Output ID

	Class: evgOutput
	Files: evegOutput.h/evgOut.cpp
	
-------------------------------------------------------------------------------
Sequencer:
-------------------------------------------------------------------------------
		VME-EVG-230 has 2 sequenceRams or sequencers. The sequenceRam can hold 
	upto 2048 <event code, timeStamp> pair. When the sequencer is triggered, an
	internal counter starts counting. When the counter value matches the timeStamp
	of the next event, the attched event code is transmitted.
	All the information that needs to be loaded into the sequenceRam to make it
 	functional, can be stored into an object called soft sequence. The IOC will have
 	number of these soft sequences but maximum of only 2 can be loaded into the
 	sequenceRam Hardware. The idea being the user can create or modify these soft
 	sequences even if 
	1.The sequence is not loaded in the sequenceRam. 
	2. The sequence is loaded into sequeneRams and is running.
 
		A soft sequence can be loaded in the hardware by using the 'load' record.
 	The user can modify any loaded soft sequence, anytime but if that sequence happens to
	be loaded, the changes are not propogated to the hardware until user commits the
 	soft sequence. 'commit' record can be used to commit the updated soft sequence
 	to the hardware. 'commit' makes sure that the sequenceRam is not modified when
 	it is active.

	Following records interact with the registers of the sequenceRam.
	Usage:
		caput EVG$(cardNum):Seq$(seqNum):load <1>
	Load the soft sequence with ID 'seqNum' into the sequenceRam hardware. For load
 	to be successful atleast one of the sequenceRam shouldn't be loaded. When
 	successful, sequenceRam enters its 'LOADED'state. If all the sequenceRam are loaded
 	it returns an error. 
		
		caput EVG$(cardNum):Seq$(seqNum):unload <1>
	Unload the soft sequence with ID 'seqNum' from the sequenceRam that it is running on.
 	When successful the sequenceRam enters its 'UNLOADED' state.

		caput EVG$(cardNum):Seq$(seqNum):commit <1>
	Commit the changes to the soft sequence with ID 'seqNum' to the sequenceRam. 
	When ever you want to make changes to the sequenceRam, you need make the changes
 	to the soft sequence that is loaded in the sequenceRam. Then 'commit' can be used
 	to progate those changes from the soft sequence to the hardware sequenceRam. Any
 	changes to the soft sequence is not written to the sequenceRam untill you 'commit'
 	that soft sequence.
	Modifing the sequenceRam while it is running gives undefined behavior hence 'commit'
 	makes sure that the changes are not written to the hardware while it is running.
 	It waits for the current sequence to finish before writing to the sequenceRam.

		caput EVG$(cardNum):Seq$(seqNum):enable <1>
	Enable the sequenceRam in which this soft sequence is loaded.	
	If sequenceRam is already loaded the record does nothing. 
	
		caput EVG$(cardNum):Seq$(seqNum):disable <1>		
	Disable the sequenceRam in which this soft sequence is loaded. If the sequence
 	is currently running the record waits for the current sequence to complete before
 	disabling it. 

		caput EVG$(cardNum):Seq$(seqNum):halt <1>
	Halt or Disable immediately the sequenceRam into which this soft sequence is
 	loaded. The difference between halt and disable is that halt does wait for the
 	current running sequence to complete, it diable the sequence ram immediately.
 	while disable allows the running sequence to complete.

	Following record are used to create and modify soft sequences. They do not directly
 	interact with the registers of the sequenceRam.

	Usage:
		caput -a EVG$(cardNum):Seq$(seqNum):eventCode <array>
	It is used to set the eventCodes of the soft sequence. These eventCodes are
 	transmitted whenever the timeStamp associated with eventCode matches the counter
 	value on sequencer. The counter on the sequencer is triggered by source selected
	by 'trigSrc'.

		caput -a EVG$(cardNum):Seq$(seqNum):timeStamp:tick <array>
	It is used to set the timeStamps for the events on the soft sequence in the
 	'Event Clock' ticks.	
 
		caput -a EVG$(cardNum):Seq$(seqNum):timeStamp:sec <array> 
	It is used to set the timeStamps for the events in the soft sequence in the seconds.

		caput EVG$(cardNum):Seq$(seqNum):runMode <mode> 
	runMode is used determine what will the sequencer do at the end of the sequence.
	where 'mode' could be any of the following:
	Single 	  - Disables the sequencer at the end of the sequence.
	Automatic - Restarts the sequence immediately after the end of the sequence.
	Normal    -	Waits for a new trigger after the end of the sequence 
				to restart the sequence. 

		caput EVG$(cardNum):Seq$(seqNum):trigSrc <src>	
	trigSrc is used to select the source of the trigger, which should start the sequencer.
	where 'src' could be any of the following:
	Mxc0 to Mxc7 - Trigger from MXC0 - MXC7
	AC			 - Trigger from AC sync logic
	RAM0/RAM1	 - Trigger from RAM0/RAM1 software trigger

	Macro: 	$(cardNum) = Logical card number EVG
			$(seqNum) = ID of the soft sequence under consideration

	Class: evgSeqRamMgr
	Files: evgSeqRamManager.h/evgSeqRamManager.cpp

	Class: evgSeqRam
	Files: evgSeqRam.h/evgSeqRam.cpp

	Class: evgSoftSeqMgr
	Files: evgSoftSeqManager.h/evgSoftSeqManager.cpp

	Class: evgSoftSeq
	Files: evgSoftSeq.h/evgSoftSeq.cpp/devEvgSoftSeq.cpp/evgSoftSeq.db

-------------------------------------------------------------------------------
Interrupt handling:
-------------------------------------------------------------------------------
	Current code does not handle interrupts. Though some infrastructure has been
 	laid out for Interrupt handling. The evgMrm class contains the ISR and callback
 	functions to handle the sequencer related interrupts irqStop0/1, irqStart0/1. 
	The irq structure associated with each inturrupt source maintains the list
	of records that needs to processed whenever the interrupt occurs from that
 	particular inturrupt source. 
