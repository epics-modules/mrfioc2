/**************************************************************************************************
|* $(TIMING)/evgMrmApp/src/evgMrm.h -- Class Definition for Event Generator Card
|*                                     With Modular Register Map
|*
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     4 November 2009
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 04 Nov 2009  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*   This header file contains the member function and data definitions for the evgMrm class.
|*   This class defined an event generator card that uses the modular register map.
|*   The evgMrm class implements the abstract EVG class.
|*
|*--------------------------------------------------------------------------------------------------
|* HARDWARE SUPPORTED:
|*   Series 2xx Event Generator Cards
|*     Modular Register Mask
|*
|*--------------------------------------------------------------------------------------------------
|* OPERATING SYSTEMS SUPPORTED:
|*   vxWorks
|*   RTEMS
|*
\*************************************************************************************************/

/**************************************************************************************************
|*                                     COPYRIGHT NOTIFICATION
|**************************************************************************************************
|*
|* THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
|* AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
|* AND IN ALL SOURCE LISTINGS OF THE CODE.
|*
|**************************************************************************************************
|*
|* This software is distributed under the EPICS Open License Agreement which
|* can be found in the file, LICENSE, included with this distribution.
|*
\*************************************************************************************************/

#ifndef EVG_MRM_HPP_INC
#define EVG_MRM_HPP_INC

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include  <queue>               // Standard C++ queue template

#include  <epicsTypes.h>        // EPICS Architecture-independent type definitions
#include  <epicsEvent.h>        // EPICS Architecture-independent event semaphore

#include  <mrfCommon.h>         // MRF Common definitions
#include  <mrfBusInterface.h>   // MRF Bus interface

#include  <evg/evg.h>           // EVG base class definition
#include  <evg/Sequence.h>      // EVG Sequence base class definition

/**************************************************************************************************/
/*  Architectural Definitions                                                                     */
/**************************************************************************************************/

//=====================
// Sequence RAM Definitions
//
#define EVG_SEQ_RAM_NUM            2    // Number of sequence RAMs per card
#define EVG_SEQ_RAM_SIZE        2048    // Number of event/timestamp pairs per sequence RAM
#define EVG_SEQ_RAM_MAX_INDEX   2046    // Maximum addressable element

/**************************************************************************************************/
/*                                  evgMrm Class Definition                                       */
/*                                                                                                */

class evgMrm: public EVG
{

/**************************************************************************************************/
/*  Public Methods                                                                                */
/**************************************************************************************************/

public:

    //=====================
    // Class Constructor
    //
    evgMrm (mrfBusInterface *BusInterface);

    //=====================
    // Class Destructor
    //
    ~evgMrm ();

    //=====================
    // Return card's bus type (VME, PMC, PCI, etc.)
    //
    inline epicsInt32 GetBusType () const {
        return (BusType);
    }//end GetBusType()

    //=====================
    // Return the logical card number
    //
    inline epicsInt32 GetCardNum () const {
        return (CardNum);
    }//end GetCardNum()

    //=====================
    // Return sub-unit value
    //
    inline epicsInt32 GetSubUnit () const {
        return (SubUnit);
    }//end GetSubUnit()

    //=====================
    // Return the sub-unit name (Slot, Index, etc.)
    //
    inline const char *GetSubUnitName () const {
        return (BusInterface->GetSubUnitName());
    }//end GetSubUnitName()

    //=====================
    // Return the Seconds per Tick value
    //
    inline epicsFloat64 GetSecsPerTick() const {
        return (SecsPerTick);
    }//end GetSecsPerTick()

    //=====================
    // Set Card's Debug Level
    //
    void SetDebugLevel (epicsInt32 level);

    //=====================
    // Event Link Clock Setters
    //
    epicsStatus SetOutLinkClockSource (epicsInt16   ClockSource);
    epicsStatus SetOutLinkClockSpeed  (epicsFloat64 ClockSpeed);
    epicsStatus SetInLinkClockSpeed   (epicsFloat64 ClockSpeed);

    //=====================
    // Card Configuration and Initialization Routines
    //
    void Configure  ();
    void RebootInit ();
    void IntEnable  ();

    //==============================================================================================
    // Sequence RAM Routines
    //==============================================================================================

    inline epicsInt32  GetSeqRamCount    () const {
        return EVG_SEQ_RAM_NUM;
    }//end GetSeqRamCount()

    inline epicsInt32  GetSeqRamSize     () const {
        return EVG_SEQ_RAM_SIZE;
    }//end GetSeqRamSize()

    inline epicsInt32  GetSeqRamMaxIndex () const {
        return EVG_SEQ_RAM_MAX_INDEX;
    }//end GetSeqRamMaxIndex()

    SequenceRAM*  GetSeqRam (epicsInt32 Ram) const;

    //=====================
    // Interrupt Handling Routine
    //
    void Interrupt  ();

    //=====================
    // Card Report Routine
    //
    epicsStatus Report (epicsInt32 level) const;

    //==============================================================================================
    // Sequence Update Routines
    //==============================================================================================

    inline epicsEvent* GetSeqUpdateEvent () const {
        return SeqUpdateEvent;
    }//end GetSeqUpdateEvent()

    inline SequenceUpdateQueue* GetSeqUpdateQueue () const {
        return SeqUpdateQueue;
    }//end GetSeqUpdateQueue()

    void UpdateSequence (Sequence* pSequence);

/**************************************************************************************************/
/*  Private Methods                                                                               */
/**************************************************************************************************/

private:
    void SetFracSynth ();


/**************************************************************************************************/
/*  Private Data                                                                                  */
/**************************************************************************************************/

private:

    //=====================
    // Card-Related Data
    //
    epicsInt32            CardNum;          // Logical card number for this card
    epicsMutex*           CardMutex;        // Mutex to lock access to the card
    epicsInt32*           DebugFlag;        // Pointer to which debug flag we should use
    epicsInt32            LocalDebugFlag;   // Card-specific debug level

    //=====================
    // Bus-Related Data
    //
    mrfBusInterface*      BusInterface;     // Address of bus interface object
    epicsInt32            BusType;          // Bus type for this card (VME, PCI, etc.)
    epicsInt32            SubUnit;          // Bus sub-unit address (e.g. VME Slot, PCI Index, etc.)

    //=====================
    // Hardware-Related Data
    //
    epicsUInt32           pReg;             // CPU Address for accessing the card's register map
    epicsUInt32           FPGAVersion;      // Firmware version number

    //=====================
    // Event Link Clock Data
    //
    epicsFloat64          OutLinkFrequency; // Event clock frequency for the outgoing link
    epicsFloat64          InLinkFrequency;  // Event clock frequency for the incoming link
    epicsFloat64          SecsPerTick;      // Seconds per event clock tick (outgoing link)
    epicsUInt32           FracSynthWord;    // Fractional synthesizer control word
    epicsInt16            OutLinkSource;    // Clock source for outgoing event link

    //=====================
    // Sequence Update Queue and Event Semaphore
    //
    epicsEvent*           SeqUpdateEvent;   // Sequence update event
    SequenceUpdateQueue*  SeqUpdateQueue;   // Sequence update queue

    //=====================
    // Sequence RAM Data
    //
    SequenceRAM*          SeqRam [EVG_SEQ_RAM_NUM+1];  // Array of sequence RAM objects;

};// end class evgMrm //

#endif // EVG_MRM_HPP_INC //
