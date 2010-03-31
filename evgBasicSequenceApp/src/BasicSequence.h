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

#include  <string>               // Standard C++ string class

#include  <epicsTypes.h>         // EPICS Architecture-independent type definitions
#include  <epicsMutex.h>         // EPICS Mutex class definition

#include  <mrfCommon.h>          // MRF Common definitions

#include  <evg/evg.h>            // Event generator base class definition
#include  <evg/Sequence.h>       // Event generator Sequence base class definition
#include  <BasicSequenceEvent.h> // Basic Sequence Event class definition


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
    // Class Destructor
    //
    ~BasicSequence ();

    //=============================================================================================
    // Required device support routines
    //=============================================================================================

    void Report   (epicsInt32 Level) const;                     // Sequence Report Function
    void Finalize ();                                           // Finalize the sequence

    //=============================================================================================
    // Sequence exclusive access routines
    //=============================================================================================

    inline void lock() {                                        // Acquire exclusive access
        SequenceMutex->lock();
    }//end lock()

    inline void unlock() {                                      // Release exclusive access
        SequenceMutex->unlock();
    }//end unlock()

    //=============================================================================================
    // General Getter Routines
    //=============================================================================================

    inline const std::string& GetClassID() const {              // Get the class ID string
        return (ClassID);
    }//end GetClassID()

    inline const char* GetSeqID() const {                       // Get the sequence ID string
        return (SeqID);
    }//end GetSeqID()

    inline epicsInt32 GetCardNum() const {                      // Get the card number
        return (CardNumber);
    }//end GetCardNum()

    inline epicsInt32 GetSeqNum () const {                      // Get sequence number as an integer
        return (SequenceNumber);
    }//end GetSeqNum()

    inline epicsInt32 GetNumEvents() const {                    // Get num events in the sequence
        return (NumEvents);
    }//end GetNumEvents()

    inline epicsFloat64 GetSecsPerTick() const {                // Get seconds / event clock tick
        return (pEvg->GetSecsPerTick());
    }//end GetSecsPerTick()

    inline const epicsInt32 *GetEventArray() const {            // Get address of event code array
        return (EventCodeArray);
    }//end GetEventArray()

    inline const epicsUInt32 *GetTimeStampArray() const {       //Get address of timestamp array
        return (TimeStampArray);
    }//end GetTimeStampArray()

    //=============================================================================================
    // Sequence Update Routines
    //=============================================================================================

    SequenceStatus   StartUpdate  ();                           // Start a Sequence object update
    void             Update       ();                           // Update the Sequence object
    void             FinishUpdate ();                           // Complete the Sequence update

    //=============================================================================================
    //  Support for "Update Mode" Records
    //=============================================================================================

    void            RegisterUpdateModeRecord (dbCommon *pRec);  // Register an "Update Mode" record
    SequenceStatus  SetUpdateMode (SequenceUpdateMode Mode);    // Set the update mode

    inline SequenceUpdateMode GetUpdateMode () const {          // Get the Update Mode
        return (UpdateMode);
    }//end GetUpdateMode()

    //=============================================================================================
    //  Support for "Update Trigger" Records
    //=============================================================================================

    void            RegisterUpdateTriggerRecord(dbCommon *pRec);// Register an "Update Trigger" Rec.

    //=============================================================================================
    //  Support for "Max TimeStamp" Records
    //=============================================================================================

    void            RegisterMaxTimeStampRecord (dbCommon *pRec);// Register a "Max TimeStamp" Rec.
    SequenceStatus  SetMaxTimeStamp (epicsFloat64 MaxTime);     // Set maximum timestamp value

    inline epicsFloat64 GetMaxTimeStamp () const {              // Get maximum timestamp value
        return MaxTimeStamp;
    }//end GetMaxTimeStamp()


/**************************************************************************************************/
/*  Public Functions (Specific To The BasicSequence Class)                                        */
/**************************************************************************************************/

public:

    //=====================
    // Class constructor
    //
    BasicSequence (epicsInt32 Card, epicsInt32 Number);

    //=====================
    // Create a new event and add it to the sequence
    //
    BasicSequenceEvent *DeclareEvent (const std::string &Name);

    //=====================
    // Get an event by name
    //
    BasicSequenceEvent *GetEvent (const std::string &Name);

    //=====================
    // Notify Sequence of an update
    //
    SequenceStatus  UpdateNotify();

    //=====================
    // Return the "Update Busy" status of the Sequence
    //
    inline bool updating () const {
        return SequenceBusy;
    }//end updating()


/**************************************************************************************************/
/*  Public Functions                                                                              */
/**************************************************************************************************/

private:

    //=====================
    // Reset the actual timestamps for all events
    //
    void ResetTimeStamps ();

/**************************************************************************************************/
/*  Private Data                                                                                  */
/**************************************************************************************************/

private:

    //=====================
    // Sequence and Sequence ID Variables
    //
    std::string          ClassID;            // Basic Sequence Class ID string
    epicsInt32           CardNumber;         // EVG card this sequence belongs to.
    epicsInt32           SequenceNumber;     // Unique sequence number for this sequence
    char                 SeqID [32];         // Sequence ID string
    EVG*                 pEvg;               // Event generator card object

    //=====================
    // Sequence Guard and State Variables
    //
    epicsMutex*          SequenceMutex;      // Mutex to guard access to this object
    bool                 SequenceBusy;       // True if sequence is being updated
    bool                 UpdatesLocked;      // True if update starts are not allowed 
    bool                 UpdateStartPending; // True if an update start is pending

    //=====================
    // Update Mode variables
    //
    SequenceUpdateMode   UpdateMode;         // Update mode specified by the update mode record
    SequenceUpdateMode   EffectiveUpdateMode;// Update mode currently in effect
    bool                 UpdateModePending;  // True if asynchronous record completion is pending
    dbCommon*            UpdateModeRecord;   // Pointer to the update mode record

    //=====================
    // Update Trigger variables
    //
    dbCommon*            UpdateTriggerRecord;// Pointer to the update trigger record

    //=====================
    // Maximum Timestamp variables
    //
    epicsFloat64         MaxTimeStamp;       // Maximum timestamp value in event clock ticks.
    bool                 MaxTimeStampPending;// True if asynchronous record completion is pending
    dbCommon*            MaxTimeStampRecord; // Pointer to the maximum timestamp record

    //=====================
    // Sequence Event Object List
    //
    epicsInt32           NumEvents;          // Number of events defined for this sequence
    BasicSequenceEvent** EventList;          // Array of sequence event pointers

    //=====================
    // Event and TimeStamp Arrays for the Sequence RAMs
    //
    epicsInt32*          EventCodeArray;     // Array of event codes
    epicsUInt32*         TimeStampArray;     // Array of time stamps

};// end class BasicSequence //

#endif // EVG_BASIC_SEQUENCE_H_INC //
