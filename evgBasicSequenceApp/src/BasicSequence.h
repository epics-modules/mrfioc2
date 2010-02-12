/**************************************************************************************************
|* $(MRF)/evgBasicSequenceApp/src/BasicSequence.h -- Event Generator BasicSequence Class Definition
|*
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     14 January 2010
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 14 Jan 2010  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*   This header file contains the definition of a BasicSequence class.
|*
|*   The BasicSequence class uses BasicSequenceEvent objects --collections of individual
|*   records that define a sequence event's properties -- to construct an event sequence
|*   that can be loaded into an event generator's sequence RAMs.
|*
|*   BasicSequences are useful for machines with single (or relatively few) timelines that
|*   have no relationships between the individual events.
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

#ifndef EVG_BASIC_SEQUENCE_H_INC
#define EVG_BASIC_SEQUENCE_H_INC

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <string>               // Standard C++ string class
#include <epicsTypes.h>         // EPICS Architecture-independent type definitions

#include <mrfCommon.h>          // MRF Common definitions

#include <evg/evg.h>            // Event generator base class definition
#include <evg/Sequence.h>       // Event generator Sequence base class definition
#include <BasicSequenceEvent.h> // Basic Sequence Event class definition


/**************************************************************************************************/
/*  Basic Sequence Class ID String                                                                */
/**************************************************************************************************/

#define  BASIC_SEQ_CLASS_ID  "BasicSequence"

/**************************************************************************************************/
/*                               Basic Sequence Class Definition                                  */
/*                                                                                                */

class BasicSequence: public Sequence
{

/**************************************************************************************************/
/*  Public Methods (Required By The Base Class)                                                   */
/**************************************************************************************************/

public:

    //=====================
    // Get the class ID string
    //
    inline const std::string& GetClassID() const {
        return (ClassID);
    }//end GetClassID()

    //=====================
    // Get the sequence ID string
    //
    inline const char* GetSeqID() const {
        return (SeqID);
    }//end GetSeqID()

    //=====================
    // Get the card number
    //
    inline epicsInt32 GetCardNum() const {
        return (CardNumber);
    }//end GetCardNum()

    //=====================
    // Get the sequence number as an integer
    //
    inline epicsInt32 GetSeqNum () const {
        return (SequenceNumber);
    }//end GetSeqNum()

    //=====================
    // Get the number of events in the sequence
    //
    inline epicsInt32 GetNumEvents() const {
        return (NumEvents);
    }//end GetNumEvents()

    //=====================
    // Get the address of the event code array
    //
    inline const epicsInt32 *GetEventArray() const {
        return (EventCodeArray);
    }//end GetEventArray()

    //=====================
    //Get the address of the timestamp array
    //
    inline const epicsUInt32 *GetTimestampArray() const {
        return (TimestampArray);
    }//end GetTimestampArray()

    //=====================
    // Finalize the sequence
    //
    void Finalize ();

    //=====================
    // Sequence Report Function
    //
    void Report (epicsInt32 Level) const;

    //=====================
    // Destructor
    //
    ~BasicSequence ();

/**************************************************************************************************/
/*  Public Methods (Specific To The BasicSequence Class)                                          */
/**************************************************************************************************/

public:

    //=====================
    // Class constructor
    //
    BasicSequence (epicsInt32 Card, epicsInt32 Number);

    //=====================
    // Get the number of seconds per event clock tick
    //
    inline epicsFloat64 GetSecsPerTick() const {
        return (pEvg->GetSecsPerTick());
    }//end GetSeqsPerTick()

    //=====================
    // Create a new event and add it to the sequence
    //
    BasicSequenceEvent *DeclareEvent (const std::string &Name);

    //=====================
    // Get an event by name
    //
    BasicSequenceEvent *GetEvent (const std::string &Name);


/**************************************************************************************************/
/*  Private Data                                                                                  */
/**************************************************************************************************/

private:

    //=====================
    // Sequence and Sequence ID variables
    //
    std::string          ClassID;            // Basic Sequence Class ID string
    epicsInt32           CardNumber;         // EVG card this sequence belongs to.
    epicsInt32           SequenceNumber;     // Unique sequence number for this sequence
    char                 SeqID [32];         // Sequence ID string
    EVG*                 pEvg;               // Event generator card object

    //=====================
    // Sequence Event Object List
    //
    epicsInt32           NumEvents;          // Number of events defined for this sequence
    BasicSequenceEvent** EventList;          // Array of sequence event pointers

    //=====================
    // Event and Timestamp Arrays for the Sequence RAMs
    //
    epicsInt32*          EventCodeArray;     // Array of event codes
    epicsUInt32*         TimestampArray;     // Array of time stamps

};// end class BasicSequence //

#endif // EVG_BASIC_SEQUENCE_H_INC //
