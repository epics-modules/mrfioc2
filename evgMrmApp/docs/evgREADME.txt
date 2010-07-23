
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
Sequence Ram:
-------------------------------------------------------------------------------
		VME-EVG-230 has 2 sequenceRams or sequencers. The sequenceRam can hold 
	upto 2048 <event code, timeStamp> pair. When the sequencer is triggered, an
	internal counter starts counting. When the counter value matches the timeStamp
	of the next event, the attched event code is transmitted.
		All the information that is needed to run the sequenceRam can be stored in a
	soft sequence. The user can maintain any number of soft sequences but at
	atime, maximum of only 2 of these soft sequences can be loaded into the
	sequenceRam Hardware. The idea being the user can create and manipulate any number of
 	these soft sequeunces irrespective of the fact whether the soft sequence is
 	actually loaded in hardware or not.
		A soft sequence can be loaded in the hardware by using the 'load' record.
 	The user can modify any soft sequence, anytime but if that sequence happens to
	be loaded, the changes are not propogated to the hardware. 'commit'
 	record can be used to commit the updated soft sequence to the hardware.
 	'commit' makes sure that the sequenceRam is not modified when it is active.

	User uses 'evgSeqRamMgr' class to intaract with the 'evgSeqRam' class, which
	actually manipulates the sequenceRam registers on EVG. It maintains a list of
 	all Soft sequences and SequenceRam objects. 

	Usage:
		-Load the soft sequence(Binary)
		caput EVG$(cardNum):Seq$(seqNum):load 1
	Load the soft sequence with ID 'seqNum' into the unloaded sequenceRam. 
	If both the sequnceRam are loaded returns an error.		


		-Unload the soft sequence(Binary)
	Unload the soft sequence with ID 'seqNum' from the sequencer.
	caput EVG$(cardNum):Seq$(seqNum):unload 1

	
		-Commit the soft sequence(Binary)		
		caput EVG$(cardNum):Seq$(seqNum):commit 1
	Commit the soft sequence with ID 'seqNum' to the sequence. 
	Commit record is used to update an sequence which is currently loaded in 
	one of the sequenceRam. When the user commits an updated sequence and if
	the old sequence is not running (i.e. the sequenceRam in which the old 
	sequence is loaded is disabled) then the new updated sequence is writen to
	the sequenceRam. 
	Modifing the sequenceRam when it is running gives undefined behavior hence
	if the old sequence is running(i.e. the sequenceRam in which the old sequence
	is loaded is running) then the commit returns but before returning it sets up
	a callback to process the commmit record again after the current old seqeuence
	reaches the end of sequence.  
	

		-Enable the soft sequence(Binary)
		caput EVG$(cardNum):Seq$(seqNum):enable 1
	Enable the soft sequence with id 'seqNum'. This basically enables
	the sequenceRam into which that soft sequence is loaded. If sequenceRam is
 	already loaded the record does nothing. 
	
			
		-Disable the soft sequence(Binary)
		caput EVG$(cardNum):Seq$(seqNum):disable 1
	Disable the soft sequence with id 'seqNum'. It disables the
	sequenceRam into which that soft sequence is loaded. If the sequence is currently
	running the record waits for the current sequence to complete and then disables it. 

	
		-Halt the soft sequence(Binary)
		caput EVG$(cardNum):Seq$(seqNum):halt 1
	Disable the soft sequence with id 'seqNum' immediately. The difference between halt
 	and disable is that halt does wait for the current running sequence to complete,
 	it diable the sequence ram immediately. while disable allows the current
	sequence to complete.


	Macro: 	$(cardNum) = Logical card number EVG
			$(seqNum) = ID of the soft sequence

	Class: evgSeqRamMgr
	Files: evgSeqRamManager.h/evgSeqRamManager.cpp/devEvgSeq.cpp/evgSeq.db
				------------------------------

	Class: evgSeqRam
	Files: evgSeqRam.h/evgSeqRam.cpp/devEvgSeq.cpp/evgSeq.db

	The EVG user doesn't deal with this class directly to manipulate the sequencer
	registers but instead uses 'evgSeqRamMgr' which in turn uses 'evgSeqRam' interface
 	to configure the  sequenceRam.

				------------------------------

	Class: evgSequence
	Files: evgSequence.h/evgSequence.cpp/devEvgSeq.cpp/evgSeq.db

	This class is used as soft sequence. 
	
	Usage:
	caput -a EVG$(cardNum):Seq$(seqNum):timeStamp array 

	caput -a EVG$(cardNum):Seq$(seqNum):eventCode array

	runMode is used determine what will the sequencer do at the end of the sequence.
	caput EVG$(cardNum):Seq$(seqNum):runMode mode 
		where 'mode' could be any of the following:
		Single 	  - Disables the sequencer at the end of the sequence.
		Automatic - Restarts the sequence immediately after the end of the sequence.
		Normal    -	Waits for a new trigger after the end of the sequence 
					 to restart the sequence. 
	
	trigSrc is used to select the source of the trigger, which should start the sequencer.
	caput EVG$(cardNum):Seq$(seqNum):trigSrc src
		where 'src' could be any of the following:
		Mxc0 to Mxc7 - Trigger from MXC0 - MXC7
		AC			 - Trigger from AC sync logic
		RAM0/RAM1	 - Trigger from RAM0/RAM1 software trigger

-------------------------------------------------------------------------------
Soft Sequence:
-------------------------------------------------------------------------------
	Soft sequence class and also soft sequence manager, which maintains the list
 	of the soft sequence.

	Soft sequence should be common to all the EVG in the IOC

-------------------------------------------------------------------------------
Interrupt handling:
-------------------------------------------------------------------------------
	Current code does not handle interrupts. Though some infrastructure has been
 	laid out for Interrupt handling. The evgMrm class contains the ISR and callback
 	functions to handle the sequencer related interrupts irqStop0/1, irqStart0/1. 
	The irq structure associated with each inturrupt source maintains the list
	of records that needs to processed whenever the interrupt occurs from that
 	particular inturrupt source. 
