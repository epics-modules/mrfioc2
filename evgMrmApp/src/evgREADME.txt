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
			caput EVG$(cardNum):Enable ena
				ena = 1 to enable the EVG
				ena = 0 to disable the EVG

			Macro: 	$(cardNum) = Logical card number EVG

-------------------------------------------------------------------------------

Event Clock:
	All the operations on EVG are synchronised to the event clock. Which is derived
	from either externally provided RF clock or from an on-board fractional synthesiser.

	 Usage:
		caput EVG$(cardNum):ClkSrc clksrc(0 to 32)
			If clksrc == 0 means clk source is internal i.e on-board fractional 
			synthesiser. The clk speed will be set to the previously requested clkspeed. 

			If clksrc > 0 means clk source is external and the clk speed will be set
			RF frequency divided by clksrc. So to have clk speed of RF/4 command will be
			caput EVG$(cardNum):ClkSrc 4	 

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
		caput EVG$(cardNum):SoftEvtEna Enable
		caput EVG$(cardNum):SoftEvtEna Disable
		caput EVG$(cardNum):SoftEvtCode eventCode(0 to 255)
			Sends out the Event Code eventCode to the event stream

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
	VME-EVG-230 has 2 sequenceRams or sequencers. The sequencer table can hold 
	upto 2048 event codes, which can be transmitted at the corresponding 32-bit
	timeStamp relative to the start of the sequence. Even though the EVG has 
	just 2 hardware sequenceRams, the driver can maintain any number of soft 
	sequences. At a time, maximum only 2 of these soft sequences can be loaded 
	into sequenceRams. The idea being the user can create and manipulate any number
	of these soft sequeunces irrespective of the fact whether the soft sequence is 
	actually loaded in hardware or not. After creation or manipulation of soft 
	sequence user can load that sequence into the hardware. Logic is build in the 
	seqRamMgr so that the sequenceRam is not modified when it is active.


	Class: evgSeqRamMgr
	Files: evgSeqRamManager.h/evgSeqRamManager.cpp/devEvgSeq.cpp/evgSeq.db

	User uses 'evgSeqRamMgr' class to intaract with the 'evgSeqRam' class, which
	actually manipulates the sequenceRam registers on EVG. It maintains a list of
 	all Soft sequences and SequenceRam objects.

	Usage:
		caput EVG$(cardNum):Seq$(seqNum):load 1
			Load the soft sequence with ID 'seqNum' into the sequencer

		caput EVG$(cardNum):Seq$(seqNum):unload 1
			Unload the soft sequence with ID 'seqNum' from the sequencer

		caput EVG$(cardNum):Seq$(seqNum):commit 1
			Commit the soft sequence with ID 'seqNum' to the sequencer

		caput EVG$(cardNum):Seq$(seqNum):enable 1
			Enable the sequencer with soft sequence ID 'seqNum'

		caput EVG$(cardNum):Seq$(seqNum):disable 1
			Disable the sequencer with soft sequence ID 'seqNum'

		Macro: 	$(cardNum) = Logical card number EVG
				$(seqNum) = ID of the soft sequence

				------------------------------

	Class: evgSeqRam
	Files: evgSeqRam.h/evgSeqRam.cpp/devEvgSeq.cpp/evgSeq.db

	The EVG user doesn't deal with this class directly to manipulate the sequencer
	registers but instead uses 'evgSeqRamMgr' which in turn uses 'evgSeqRam' interface.

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
