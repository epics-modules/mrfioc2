---------------Epics device driver for MRF VME-EVG-230-------------------------

Author: Jayesh Shah, NSLS2, BNL
		jshah@bnl.gov

Acknowlegement:
	Parts of the MRF VME-EVG-230 device driver code has been reused from 
		- Epics device driver for MRF EVR-230 by Micheal Davidsaver, NSLS2, BNL.
		- Epics device driver for MRF EVG-230 by Eric Bjorklund, LANSCE, LANL.


First read evgTODO.txt for the list of the things expected to work in the
checkout code.

TODO: Introduction to Timing System
TODO: Introduction to EVG
TODO: Introduction to EVR 

Brief note about all the hardware/software components of VME-EVG-230
-------------------------------------------------------------------------------

EVG:
	Class: evgMrm
	Files: evgMrm.h/evgMrm.cpp/devEvgMrm.cpp/evgMrm.db

	Each EVG will be represented by the instance of class 'evgMrm'. Each evg 
	object maintains a reference to all the evg sub-componets i.e. Event clock,
	Software Events, Trigger Events, Distributed bus, Multiplex Counters, 
	Sequence Ram, Front Panel I/O etc.
	
	Usage:
		-Enable/Disable the EVG (Binary)
		caput EVG$(cardNum):Enable Enable
		caput EVG$(cardNum):Enable Disable

	Macro: 	$(cardNum) = Logical card number EVG

-------------------------------------------------------------------------------

Event Clock:
	All the operations on EVG are synchronised to the event clock. Which is derived
	from either externally provided RF clock or from an on-board fractional synthesiser.

	 Usage:
		-Set the event clock source  (longout)
		caput EVG$(cardNum):ClkSrc clksrc(0 to 32)
		If clksrc == 0 means clk source is internal i.e on-board fractional synthesiser.
 	The clk speed will be set to the previously requested clkspeed. 
		If clksrc > 0 means clk source is external and the clk speed will be set RF 
	frequency divided by clksrc. So to have clk speed of RF/4 command will be
	caput EVG$(cardNum):ClkSrc 4	 
		
		-Set the event clock frequency (longout)
		caput EVG$(cardNum):ClkSpeed clkspeed(50 to 125)
	This pv is used to set the clk speed if the clk source is internal(i.e. 0).
	If the clk source is external(i.e > 0) then the requested clk speed is recorded 
	and is used whenever the Clk Source changes to internal.

	Macro: 	$(cardNum) = Logical card number EVG
	
-------------------------------------------------------------------------------

Software Events:
	Class: evgMrm
	Files: evgMrm.h/evgMrm.cpp/devEvgMrm.cpp/evgMrm.db

	Software event is used to send out an event code by writing that event to a
	particular register in EVG.

	Usage:
		-Enable/Disable the soft event transmission(Binary)
		caput EVG$(cardNum):SoftEvtEna Enable
		caput EVG$(cardNum):SoftEvtEna Disable

		-Sent Soft Event code(longout)
		caput EVG$(cardNum):SoftEvtCode eventCode(0 to 255)
	Sends out the Event Code eventCode to the event stream.

	Macro: 	$(cardNum) = Logical card number EVG

-------------------------------------------------------------------------------

Trigger Events:
	Class: evgTrigEvt
	Files: evgTrigEvt.h/evgTrigEvt.cpp/devEvgTrigEvt.cpp/evgTrigEvt.db

	There are currently 8 trigger event sources. Trigger events are used to send
	out event code into the event streams. The event code transmitted is determined 
	by the contents of the corresponding event code register. The stimulus may
 	be a detected rising edge on an external signal or a rising edge of a 
	multliplexed counter output.

	Usage:
		caput EVG$(cardNum):TrigEvt$(trigEvtNum)Code eventCode(0 to 255)
		caput EVG$(cardNum):TrigEvt$(trigEvtNum)Ena Enable
		caput EVG$(cardNum):TrigEvt$(trigEvtNum)Ena Disable

		Macro: 	$(cardNum) = Logical card number EVG
				$(trigEvtNum) = The trigger event ID(0 to 7)

-------------------------------------------------------------------------------

Distributed bus:
	Class: evgDbus
	Files: evgDbus.h/evgDbus.cpp/devEvgDbus.cpp/evgDbus.db

	The bits of the distributed bus are obtained by any of the three methods
	1. Sampling the external input signals at event rate
	2. Forwarding the output of corresponding multiplexed counter
	3. Forwarding the state of all distributed bus bits of upstram EVG.
	There are in all 8 distributed bus bits.

	The device support uses mboo record to select the source of signals 
	transmitted by distributed bus of EVG. 

	Usage:
		caput EVG$(cardNum):Dbus$(dbusBit)Map Off
		caput EVG$(cardNum):Dbus$(dbusBit)Map External
		caput EVG$(cardNum):Dbus$(dbusBit)Map MXC
		caput EVG$(cardNum):Dbus$(dbusBit)Map UpstreamEVG
		
		Macro: 	$(cardNum) = Logical card number EVG
				$(dbusBit) = The distributed bus bit(0 to 7)

-------------------------------------------------------------------------------

Multiplexed Counter:
	Class: evgMxc
	Files: evgMxc.h/evgMxc.cpp/devEvgMxc.cpp/evgMxc.db

	There are 8 32-bti multiplexed counters that generate clock signals with 
	programmable frequencies from event clock/2^32-1 to event clock/2. 
	The counter outputs may be programmed to trigger events, drive distributed
	bus signals and trigger sequence RAMs. 

	The value in prescaler register of MXC determines the output frequency of 
	the MXC. 

	Usage:
		caput EVG$(cardNum):Mxc$(mxcNum)Presacler prescaler(2) 
	For output frequency of (event clock/2) the prescaler should be 2.
			 					:
		caput EVG$(cardNum):Mxc$(mxcNum)Presacler prescaler(2^32-1) 
	For output frequency of (event clock/2^32-1) the prescaler should be 2^32-1.
		
		caput EVG$(cardNum):Mxc$(mxcNum)TrigEvtMap.B0 = 1
	Map rising edge of MXC mxcNum to send out trigger event 0
								:
								:
		caput EVG$(cardNum):Mxc$(mxcNum)TrigEvtMap.B7 = 1
	Map rising edge of MXC mxcNum to send out trigger event 7

	Macro: 	$(cardNum) = Logical card number EVG
			$(mxcNum) = ID for multiplexed counter(0 to 7)

-------------------------------------------------------------------------------

Front Panel Input/Output:
	Class: evgFPio
	Files: evgFPio.h/evgFPio.cpp/devEvgFPio.cpp/evgFPio.db
	
	The MRF VME-EVG-230 has 2 front panel inputs and 4 front panel outputs.
	The class is right now rudiment of final interface. 

	Usage:
		caput EVG$(cardNum):$(ioType)$(ioNum)Map map

		Macro: 	$(cardNum) = Logical card number EVG
				$(ioType) = FP_Input, FP_Output
				$(ioNum) = Input/Output Number(0 to 7)		

-------------------------------------------------------------------------------

Sequence Ram:
		VME-EVG-230 has 2 sequenceRams or sequencers. The sequenceRam can hold 
	upto 2048 event code, timeStamp pair. When the sequencer is triggered, an
	internal counter starts counting. When the counter value matches the timeStamp
	of the next event, the attched event code is transmitted.
		All the information that is needed to run the sequenceRam can be stored in a
	soft sequence. The user can maintain any number of soft sequences but at
	atime, maximum of only 2 of these soft sequences can be loaded into the
	sequenceRams. The idea being the user can create and manipulate any number of
 	these soft sequeunces irrespective of the fact whether the soft sequence is
 	actually loaded in hardware or not.
		A soft sequence can be loaded in the hardware by using the 'load' record.
 	The user can modify any soft sequence anytime but if that sequence happens to
	be loaded, the changes are not directly propogated to the hardware. 'commit'
 	record can be used to commit the changes to the hardware. commit makes sure that
 	the sequenceRam is not modified when it is active.
		The seqRamMgr or sequenceRam itself does not do any checking on the soft
	sequence passed to them. The soft sequence class should be doing all the error
 	checking. For example there should not be any collision in timeStamp of the events
 	in the soft sequence. It should also make sure that the last event in the
	sequence is the end of sequence(i.e. 0x7f). Also the number of event code and
 	timeStamp in the soft sequence should be the same. Currently the code does not
 	include a proper soft sequence implementation, just a basic one, to test the
 	seqRamMgr and sequenceRam. 


	Class: evgSeqRamMgr
	Files: evgSeqRamManager.h/evgSeqRamManager.cpp/devEvgSeq.cpp/evgSeq.db

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
	Disable the soft sequence with id 'seqNum'. This basically disables the
	sequenceRam into which that soft sequence is loaded. If the sequence is currently
	running the record does not wait for the current sequence to complete is just
	disables the sequenceRam. 
	
	Macro: 	$(cardNum) = Logical card number EVG
			$(seqNum) = ID of the soft sequence

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
		caput -a EVG$(cardNum):Seq$(seqNum):timeStamp 

		caput -a EVG$(cardNum):Seq$(seqNum):eventCode

		caput EVG$(cardNum):Seq$(seqNum):runMode Single/Automatic/Normal
			runMode is used determine what will the sequencer do at the end 
			of the sequence.
			Single - Disables the sequencer at the end of the sequence.
			Automatic - Restarts the sequence immediately after the end of the sequence.
			Normal - Waits for a new trigger after the end of the sequence 
					 to restart the sequence. 

-------------------------------------------------------------------------------

Interrupt handling:
	Current code does not handle interrupts. Though some infrastructure has been
 	laid out for Interrupt handling. The evgMrm class contains the ISR and callback
 	functions to handle the sequencer related interrupts irqStop0/1, irqStart0/1. 
	The irq structure associated with each inturrupt source maintains the list
	of records that needs to processed whenever the interrupt occurs from that
 	particular inturrupt source. 
