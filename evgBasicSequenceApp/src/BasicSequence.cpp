/**************************************************************************************************
|* $(MRF)/evgBasicSequenceApp/src/BasicSequence.cpp -- Event Generator Basic Sequence Class
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     19 November 2009
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 18 Nov 2009  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*   This module contains the implementation of the BasicSequence class
|*
|*   The BasicSequence class uses BasicSequenceEvent objects --collections of individual
|*   records that define a sequence event's properties -- to construct an event sequence
|*   that can be loaded into an event generator's sequence RAMs.
|*
|*   BasicSequences are useful for machines with single (or relatively few) timelines that
|*   have no relationships between the individual events.
|*
\**************************************************************************************************/

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

/**************************************************************************************************/
/*  BasicSequence Class Description                                                               */
/**************************************************************************************************/

//==================================================================================================
//! @addtogroup Sequencer
//! @{
//!
//==================================================================================================
//! @class      BasicSequence
//! @brief      Event Generator BasicSequence Class.
//!
//! @par Description:
//!    A \b BasicSequence object represents a list of events and timestamps.  The timestamp
//!    determines when the event should be delivered relative to the start of the sequence.
//!
//!    @par
//!    Although sequences are associated with event generator cards, A sequence is an abstract
//!    concept and does not in itself reference any hardware.   An event generator may "posses"
//!    mulitple sequences.  Each BasicSequence must have a unique ID number.  The ID number is
//!    assigned when the BasicSequence object is created. A BasicSequence object only becomes
//!    active when it is assigned to a "Sequence RAM". The same BasicSequence object may not be
//!    assigned to more than one Sequence RAM.
//!
//!    @par
//!    BasicSequences are useful for machines with single (or relatively few) timelines that
//!    have no relationships between the individual events.
//!
//==================================================================================================

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include  <stdexcept>            // Standard C++ exception class
#include  <string>               // Standard C++ string class

#include  <stdlib.h>             // Standard C library
#include  <stdio.h>              // Standard C I/O library
#include  <math.h>               // Standard C math library

#include  <epicsTypes.h>         // EPICS Architecture-independent type definitions
#include  <epicsMutex.h>         // EPICS Mutex class definition
#include  <dbLock.h>             // EPICS Record locking routine definitions
#include  <recSup.h>             // EPICS Record support entry table definitions

#include  <mrfCommon.h>          // MRF Common definitions
#include  <drvEvg.h>             // MRF Event Generator driver infrastructure routines
#include  <BasicSequence.h>      // MRF BasicSequence class definition
#include  <BasicSequenceEvent.h> // MRF BasicSequenceEvent class definition


/**************************************************************************************************/
/*  Forward Declarations                                                                          */
/**************************************************************************************************/

static void Sort   (BasicSequenceEvent* EventArray[], const epicsInt32 Size);
static void SiftUp (BasicSequenceEvent* EventArray[], const epicsInt32 Start);


/**************************************************************************************************/
/*  Type Definititions                                                                            */
/**************************************************************************************************/

//=====================
// Record Processing Function
//
typedef epicsStatus (*REC_PROC_FN) (dbCommon*);

/**************************************************************************************************/
/*                         BasicSequence Class Constructors and Destructors                       */
/*                                                                                                */


//**************************************************************************************************
//  BasicSequence () -- Class Constructor
//**************************************************************************************************
//! @par Description:
//!   Class Constructor
//!
//! @par Function:
//!   Create an event generator BasicSequence object and assign it to the specified event generator
//!   card using the specified sequence ID number.
//!
//! @param Card    = (input) Event generator logical card number
//! @param Number  = (input) Sequence ID number.
//!
//! @throw         runtime_error is thrown if the event generator card was not configured.
//!
//**************************************************************************************************

BasicSequence::BasicSequence (epicsInt32 Card, epicsInt32  Number):

    //=====================
    // Initialize the data members
    //
    ClassID             (BASIC_SEQ_CLASS_ID),           // Class ID string
    CardNumber          (Card),                         // EVG logical card number
    SequenceNumber      (Number),                       // Sequence number

    SequenceBusy        (false),                        // Sequence busy flag
    UpdatesLocked       (true),                         // Update starts initially not allowed
    UpdateStartPending  (false),                        // No update start is pending.

    UpdateMode          (SeqUpdateMode_Immediate),      // Default update mode
    EffectiveUpdateMode (SeqUpdateMode_OnTrigger),      // Update mode is initially "On Trigger"
    UpdateModePending   (false),                        // Asynch completion pending for Update Mode
    UpdateModeRecord    (NULL),                         // Pointer to Update Mode record

    UpdateTriggerRecord (NULL),                         // Pointer to Update Trigger record

    MaxTimeStamp        (0.0),                          // Max timestamp value in event clock ticks
    MaxTimeStampPending (false),                        // Asynch completion pending for Max Time
    MaxTimeStampRecord  (NULL),                         // Pointer to Max Timestamp record

    NumEvents           (0),                            // Number of events in this sequence
    EventList           (NULL),                         // List of event objects

    EventCodeArray      (NULL),                         // Event code array
    TimeStampArray      (NULL)                          // Event timestamp array
{
    //=====================
    // Create the sequence ID string
    //
    sprintf (SeqID, "Card %d, Seq %d", Card, Number);

    //=====================
    // Try to get the EVG object for the card we belong to
    //
    if (NULL == (pEvg = EgGetCard(Card)))
        throw std::runtime_error (std::string(SeqID) + ": Event generator card not configured");

    //=====================
    // Create and initialize the sequence event array
    //
    epicsInt32 SeqRamSize = pEvg->GetSeqRamSize();
    EventList = (BasicSequenceEvent**) calloc (SeqRamSize, sizeof(BasicSequenceEvent*));
    if (NULL == EventList)
        throw std::runtime_error (std::string(SeqID) + ": Insufficient memory");

    for (int i = 0;  i < pEvg->GetSeqRamSize();  i++)
        EventList[i] = NULL;

    //=====================
    // Create the event code array
    //
    EventCodeArray = (epicsInt32*) calloc (SeqRamSize, sizeof(epicsInt32));
    if (NULL == EventCodeArray) {
        free (EventList);
        throw std::runtime_error (std::string(SeqID) + ": Insufficient memory");
    }//end if could not create the event code array

    //=====================
    // Create the timestamp array
    //
    TimeStampArray = (epicsUInt32*) calloc (SeqRamSize, sizeof(epicsUInt32));
    if (NULL == TimeStampArray) {
        free (EventList);
        free (EventCodeArray);
        throw std::runtime_error (std::string(SeqID) + ": Insufficient memory");
    }//end if could not create the timestamp array

    //=====================
    // Create the Sequence Guard Mutex
    //
    try {SequenceMutex = new epicsMutex();}
    catch (std::exception& e) {
        free (EventList);
        free (EventCodeArray);
        throw std::runtime_error (std::string(SeqID) + ": Unable to create sequence mutex");
    }//end if could not create the guard mutex

}//end Constructor

//**************************************************************************************************
//  ~BasicSequence() -- Class Destructor
//**************************************************************************************************
//! @par Description:
//!   Free up all resources owned by this object before it is destroyed.
//!
//! @par Member Variables Referenced:
//! - \e     NumEvents      = (input)     The number of sequence events in this sequence
//! - \e     EventList      = (destroyed) Deallocate the list of BasicSequenceEvent objects
//!                                       assigned to this sequence.
//! - \e     EventCodeArray = (destroyed) Deallocate the event code array for this sequence.
//! - \e     TimeStampArray = (destroyed) Deallocate the timestamp array for this sequence.
//! - \e     SequenceMutex  = (destroyed) Deallocate the mutex guarding this sequence.
//!
//**************************************************************************************************

BasicSequence::~BasicSequence () {

    //=====================
    // Delete the sequence events and the sequence event list
    //
    for (epicsInt32 i = 0;  i < NumEvents;  i++)
        if (NULL != EventList[i]) delete EventList[i];
    free (EventList);

    //=====================
    // Delete the sequence arrays
    //
    free (EventCodeArray);
    free (TimeStampArray);

    //=====================
    // Delete the guard mutex
    //
    delete (SequenceMutex);

}//end destructor

/**************************************************************************************************/
/*                              Required Device Support Routines                                  */
/*                                                                                                */

//**************************************************************************************************
//  Report () -- Display Information About This Sequence
//**************************************************************************************************
//! @par Description:
//!   This routine displays information about a BasiceSequence object.  The information displayed
//!   is (depending on the requested display level):
//!   - Card number of the EVG the sequence belongs to.
//!   - Sequence number
//!   - Sequence type (e.g. BasicSequence)
//!   - Information on each event in the sequence
//!
//! @param   Level     = (input) Report detail level<br>
//!                              Level  = 0: No Report<br>
//!                              Level >= 1: Display the sequence number and type<br>
//!                              Level >= 2: Display the sequence events
//!
//! @par Member Variables Referenced:
//! - \e     ClassID   = (input) Sequence class ID string (e.g. "BasicSequence")
//! - \e     EventList = (input) List of BasicSequenceEvent objects in this sequence
//! - \e     NumEvents = (input) Number of events in this sequence
//! - \e     SeqID     = (input) Sequence ID string (contains card & sequence number)
//!
//**************************************************************************************************

void
BasicSequence::Report (epicsInt32 Level) const {

    //=====================
    // If Level >= 1, Display the sequence number and type
    //
    if (Level < 1) return;
    printf ("  %s (%s):  Update Mode = ", SeqID, ClassID.c_str());

    //=====================
    // Display the sequence update mode
    //
    switch (UpdateMode) {
    case SeqUpdateMode_Immediate:
        printf ("Immediate");
        break;
    case SeqUpdateMode_OnTrigger:
        printf ("On Trigger");
        break;
    default:
        printf ("Unknown");

    }//end switch on update mode

    //=====================
    // Display the sequence's maximum timestamp value
    //
    printf (":  Max TimeStamp = ");
    if (0.0 == MaxTimeStamp) printf ("None\n");
    else                     printf ("%1.0f (ticks)\n", MaxTimeStamp);

    //=====================
    // If Level >= 2, Display the sequence events
    //
    if (Level < 2) return;

    printf ("    Event  Enable   Time Stamp (ticks)  Priority  Name\n");
    for (epicsInt32 i = 0;  i < NumEvents;  i++)
        EventList[i]->Report();

    printf ("\n");

}//end Report()

//**************************************************************************************************
//  Finalize () -- Complete the BasicSequence Object Initialization
//**************************************************************************************************
//! @par Description:
//!   This routine is called during the "After Interrupt Accept" phase of the iocInit() process.
//!   It performs completes the the BasicSequence object initialization after all the sequence
//!   records and sequence event records have been initially processed.
//!
//! @par Function:
//! - Perform a "Sanity Check" on each of the BasicSequenceEvent objects in this sequence.
//!   Discard those events that don't pass the sanity check.  
//! - Set the default update mode depending on whether or not the "Update Mode" and "Update Trigger"
//!   records were specified.
//! - Schedule an update for this Sequence.
//!
//! @par Member Variables Referenced:
//! - \e NumEvents           = (modified) Number of events in this sequence
//! - \e EventList           = (modified) List of BasicSequenceEvent objects in this sequence
//! - \e EffectiveUpdateMode = (modified) Indicates the Sequence object's "Effective" update
//!                                       mode (which may be different than the update mode
//!                                       specified in the "Update Mode" record).
//! - \e EffectiveUpdateMode = (modified) Current update mode for this sequence
//! - \e UpdateMode          = (modified) Specified update mode for this sequence
//! - \e UpdatesLocked       = (modified) Allow Sequence updates to start
//!
//**************************************************************************************************

void
BasicSequence::Finalize () {

    //=====================
    // Local Variables
    //
    epicsInt32  index;          // Local index variable
    epicsInt32  listEnd;        // Index of last element in the event list.

    //=====================
    // Run a sanity check on each event in our list.
    // Discard the events that don't pass.
    //
    listEnd = NumEvents - 1;
    for (index = 0;  index < NumEvents;) {

        if (OK == (EventList[index])->SanityCheck())
            index++;

        //=====================
        // If the current event did not pass its sanity check,
        // delete the event, replace it with the last event in the list,
        // and decrement the event count.
        else {
            delete EventList[index];
            EventList[index] = EventList[listEnd];
            EventList[listEnd] = NULL;
            NumEvents--;
            listEnd--;
        }//end if we discarded an event

    }//end for each event in our list

    //=====================
    // Set a default value for the UpdateMode in the event that one or both of the
    // "Update Mode" or "Update Trigger" records are absent.
    //
    if (NULL == UpdateTriggerRecord)    // No Trigger Record: Mode is "Immediate"
        UpdateMode = SeqUpdateMode_Immediate;

    else if (NULL == UpdateModeRecord)  // Trigger Record but no Mode Record:  Mode is "On Trigger"
        UpdateMode = SeqUpdateMode_OnTrigger;
    
    //=====================
    // Clear the "Initializing" flag and set the effective update mode to the actual update mode
    //
    UpdatesLocked = false;
    EffectiveUpdateMode = UpdateMode;

    //=====================
    // Complete the sequence intialization by scheduling an update
    //
    StartUpdate();

}//end Finalize();

/**************************************************************************************************/
/*                              Support for "Update Mode" Records                                 */
/*                                                                                                */


//**************************************************************************************************
//  RegisterUpdateModeRecord () -- Register The Existence Of A BasicSequenc Update Mode Record
//**************************************************************************************************
//! @par Description:
//!   Declare the existence of a BasicSequence update mode record
//!
//! @par Function:
//!   First check to see if an update mode record has already been registered for this sequence
//!   If so, throw a runtime_error containing the name of the previously registered record.
//!   If this is the first record to register, make it the official update mode record.
//!
//! @param   pRec  = (input) Pointer to the record we wish to register
//!
//! @throw   runtime_error is thrown if an update mode record was already registered.
//!
//! @par Member Variables Referenced:
//! - \e     UpdateModeRecord = (modified) Points to the update mode record registered for
//!                              this sequence.
//!
//**************************************************************************************************

void
BasicSequence::RegisterUpdateModeRecord (dbCommon *pRec)
{
    //=====================
    // Make this the official update mode record if it was the first to be declared
    //
    if (NULL == UpdateModeRecord)
        UpdateModeRecord = pRec;

    //=====================
    // Throw an error if we already have an update mode record
    //
    else throw std::runtime_error (
        std::string (SeqID) +
        std::string (": Duplicate 'Update Mode' records declared for Sequence") +
        std::string ("'\nPreviously declared record is: ") + UpdateModeRecord->name);

}//end RegisterUpdateModeRecord()

//**************************************************************************************************
//  SetUpdateMode () -- Set the Update Mode for the Sequence
//**************************************************************************************************
//! @par Description:
//!   Determine whether the Sequence will update on every change ("Immediate" mode) or accumulate
//!   changes until a trigger occurs ("On Trigger" mode).  If the Sequence is currently being
//!   updated, the new update mode will not be set. In this case, the routine will return the
//!   "SeqStat_Busy" status, indicating that the caller should re-try the operation once the
//!   Sequence object has finished its update.
//!
//! @param   Mode       = (input) SeqUpdateMode_Immediate: Sequence will update on every change<br>
//!                               SeqUpdateMode_OnTrigger: Sequence will only update when an
//!                              "Udate Trigger" record fires.
//!
//! @return  Returns \b SeqStat_Busy  if the sequence is currently updating.<br>
//!          Returns \b SeqStat_Idle  if the Sequence was not updating.<br>
//!          Returns \b SeqStat_Error if we could not set the requested mode.
//!
//! @par Member Variables Referenced:
//! - \e EffectiveUpdateMode = (modified) Indicates the Sequence object's "Effective" update
//!                                       mode (which may be different than the update mode
//!                                       specified in the "Update Mode" record).
//! - \e SequenceBusy        = (input)    True if the Sequence is currently updating.
//! - \e UpdateMode          = (modified) The Sequence object's current operating mode
//! - \e UpdatesLocked       = (modified) Allow Sequence updates to start
//!
//! @note
//! - This routine should only be called when the Sequence object is locked.   
//!
//**************************************************************************************************

SequenceStatus
BasicSequence::SetUpdateMode (SequenceUpdateMode Mode)
{
    //=====================
    // Make sure the state is legal
    //
    if ((Mode < 0) || (Mode > SeqUpdateMode_Max))
        return SeqStat_Error;

    //=====================
    // Don't set the update mode to "On Trigger" if there is no trigger record
    //
    if ((SeqUpdateMode_OnTrigger == Mode) && (NULL == UpdateTriggerRecord))
        return SeqStat_Error;

    //=====================
    // Don't change the mode while the sequence is being updated
    //
    if (SequenceBusy) {
        UpdateModePending = true;
        return SeqStat_Busy;
    }//end if Sequence is updating

    //=====================
    // Sequence is Idle, change the update mode
    //
    UpdateMode = Mode;

    //=====================-
    // If we are not initializing or updating, change the effective update mode too.
    if (!UpdatesLocked)
        EffectiveUpdateMode = Mode;

    return SeqStat_Idle;

}//end SetUpdateMode()

/**************************************************************************************************/
/*                             Support for "Update Trigger" Records                               */
/*                                                                                                */


//**************************************************************************************************
//  RegisterUpdateTriggerRecord () -- Register The Existence Of A BasicSequence Update Trigger Rec.
//**************************************************************************************************
//! @par Description:
//!   Declare the existence of a BasicSequence update trigger record
//!
//! @par Function:
//!   First check to see if an update trigger record has already been registered for this sequence
//!   If so, throw a runtime_error containing the name of the previously registered record.
//!   If this is the first record to register, make it the official update trigger record.
//!
//! @param   pRec  = (input) Pointer to the record we wish to register
//!
//! @throw   runtime_error is thrown if an update trigger record was already registered.
//!
//! @par Member Variables Referenced:
//! - \e     UpdateTriggerRecord = (modified) Points to the update trigger record registered for
//!                                 this sequence.
//!
//**************************************************************************************************

void
BasicSequence::RegisterUpdateTriggerRecord (dbCommon *pRec)
{
    //=====================
    // Make this the official update trigger record if it was the first to be declared
    //
    if (NULL == UpdateTriggerRecord)
        UpdateTriggerRecord = pRec;

    //=====================
    // Throw an error if we already have an update trigger record
    //
    else throw std::runtime_error (
        std::string (SeqID) +
        std::string (": Duplicate 'Update Trigger' records declared for Sequence") +
        std::string ("'\nPreviously declared record is: ") + UpdateTriggerRecord->name);

}//end RegisterUpdateTriggerRecord()

/**************************************************************************************************/
/*                             Support for "Max TimeStamp" Records                                */
/*                                                                                                */

//**************************************************************************************************
//  RegisterMaxTimeStampRecord () -- Register The Existence Of A BasicSequence Max TimeStamp Record
//**************************************************************************************************
//! @par Description:
//!   Declare the existence of a BasicSequence maximum timestamp record
//!
//! @par Function:
//!   First check to see if a maximum timestamp record has already been registered for this sequence
//!   If so, throw a runtime_error containing the name of the previously registered record.
//!   If this is the first record to register, make it the official maximum timestamp record.
//!
//! @param   pRec  = (input) Pointer to the record we wish to register
//!
//! @throw   runtime_error is thrown if a maximum timestamp record was already registered.
//!
//! @par Member Variables Referenced:
//! - \e     MaxTimeStampRecord = (modified) Points to the maximum timestamp record registered for
//!                                this sequence.
//!
//**************************************************************************************************

void
BasicSequence::RegisterMaxTimeStampRecord (dbCommon *pRec)
{
    //=====================
    // Make this the official maximum timestamp record if it was the first to be declared
    //
    if (NULL == MaxTimeStampRecord)
        MaxTimeStampRecord = pRec;

    //=====================
    // Throw an error if we already have a maximum timestamp record
    //
    else throw std::runtime_error (
        std::string (SeqID) +
        std::string (": Duplicate 'Max TimeStamp' records declared for Sequence") +
        std::string ("'\nPreviously declared record is: ") + MaxTimeStampRecord->name);

}//end RegisterMaxTimeStampRecord()

//**************************************************************************************************
//  SetMaxTimeStamp () -- Set the Maximum TimeStamp Value for the Sequence
//**************************************************************************************************
//! @par Description:
//!   Sets the maximum allowed timestamp value (in event clock ticks) for this sequence.  This
//!   prevents the sequence from expanding beyond its allotted execution time and possibly
//!   getting out of synch.  Any events whose timestamps exceed the maximum timestamp value
//!   will be assigned an actual value of MaxTimeStamp +1 and will not be included in the
//!   the events written to the sequence RAM.  A maximum timestamp value of 0 indicates that there
//!   is no limit.
//!
//! @param   MaxTime      = (input) Maximum timestamp (in event clock ticks) that any event
//!                                 in the Sequence may have.  Note that MaxTime is specified
//!                                 as a 64-bit floating point number since the Sequence
//!                                 timstamps may exceed 32-bits.
//!
//! @return  Returns \b SeqStat_Busy  if the sequence is currently updating.<br>
//!          Returns \b SeqStat_Idle  if the Sequence was not updating.<br>
//!          Returns \b SeqStat_Error if we could not set the requested mode.
//!
//! @par Member Variables Referenced:
//! - \e     SequenceBusy = (input)    True if the Sequence is currently updating.
//! - \e     MaxTimeStamp = (modified) Sequence update mode flag.
//!
//! @note
//! - This routine should only be called when the Sequence object is locked.   
//!
//**************************************************************************************************

SequenceStatus
BasicSequence::SetMaxTimeStamp (epicsFloat64 MaxTime) {

    //=====================
    // Don't change the maximum timestamp while the sequence is being updated
    //
    if (SequenceBusy) {
        MaxTimeStampPending = true;
        return SeqStat_Busy;
    }//end if Sequence is updating

    //=====================
    // Sequence is Idle, change the maximum timestamp value
    // and possibly start a sequence update.
    //
    MaxTimeStamp = floor(MaxTime + 0.5);
    return UpdateNotify();

}//end SetMaxTimeStamp()

/**************************************************************************************************/
/*                                    Sequence Update Functions                                   */
/*                                                                                                */

//**************************************************************************************************
//  StartUpdate () -- Start a Sequence Update
//**************************************************************************************************
//! @par Description:
//!   Starts a BasicSequence object update.
//!
//! @par Function:
//! - If the Sequence is busy with a previous update, return the 'SeqStat_Busy' status code.
//! - Queue the Sequence object to the "Sequence Update Task" for this EVG and set the
//!   'SequenceBusy' flag.
//!
//! @return  Returns \b SeqStat_Start if the Sequence update was started.<br>
//!          Returns \b SeqStat_Busy  if the Sequence was already updating.
//!
//! @par Member Variables Referenced:
//! - \e     SequenceBusy       = (modified) True if the Sequence is updating.
//! - \e     UpdatesLocked      = (input)    True if update starts are blocked
//! - \e     UpdateStartPending = (modified) True if an update start is pending
//!
//! @note
//! - This function is designed to be called from the BasicSequence bo record device support
//!   "write" routine 
//! - This function should only be called when the Sequence object is locked.   
//!
//**************************************************************************************************

SequenceStatus
BasicSequence::StartUpdate ()
{
    //=====================
    // Sequence is still executing a previous update
    // or update starts are not allowed
    //
    if (SequenceBusy || UpdatesLocked) {
        UpdateStartPending = true;
        return SeqStat_Busy;
    }//end if sequence is still executing an update

    //=====================
    // Sequence is idle and update starts are allowed,
    // start an update
    //
    SequenceBusy = true;
    UpdateStartPending = false;
    pEvg->UpdateSequence(this);
    return SeqStat_Start;

}//end StartUpdate()

//**************************************************************************************************
//  Update () -- Perform A BasicSequence Object Sort
//**************************************************************************************************
//! @par Description:
//!   This routine is called from within the event generator's "Sequence Update Task", which
//!   runs at a priority slightly lower than EPICS record processing.  It sorts the
//!   BasicSequenceEvent objects, resolves any conflicting timestamps, and loads the sequence
//!   RAM from the Sequence object.
//!
//! @par Function:
//! - Sort the BasicSequenceEvent objects according to their timestamps.
//! - If this Sequence is assigned to a sequence RAM, update the sequence RAM.
//!
//! @par Member Variables Referenced:
//! - \e NumEvents           = (input)    Number of events in this sequence
//! - \e EventList           = (modified) List of BasicSequenceEvent objects in this sequence
//!
//**************************************************************************************************

void
BasicSequence::Update ()
{
    //=====================
    // Local Variables
    //
    epicsFloat64         CurrentTimeStamp;      // Timestamp for the current event
    epicsFloat64         increment;             // Timestamp increment for collisions
    epicsFloat64         LastTimeStamp;         // Timestamp for the previous event
    BasicSequenceEvent*  pEvent;                // Pointer to the current BasicSequenceEvent object
    bool                 SortNeeded;            // True if event list needs to be sorted.

    //=====================
    // Reset the actual timestamp of each event back to its requested value
    //
    ResetTimeStamps();

    //=====================
    // Repeat this loop until each event has a unique timestamp
    //
    SortNeeded = true;
    while (SortNeeded) {

        //=====================
        // Sort the event list by ascending timestamp and descending priority
        //
        Sort (EventList, NumEvents);
        SortNeeded = false;
        LastTimeStamp = -1.0;
        increment = 1.0;

        //=====================
        // Search the sorted event list for duplicate timestamps
        //
        for (epicsInt32 i = 0;  i < NumEvents;  i++) {
            pEvent = EventList[i];

            //=====================
            // Only look at enabled events with legal timestamps
            //
            if (pEvent->Enabled() && pEvent->TimeStampLegal()) {
                CurrentTimeStamp = pEvent->GetActualTime();

                //=====================
                // If we find a duplicate timestamp, we know from the sort algorithm
                // that the current event's priority is less than or equal to the previous
                // event.  Jostle the current event by incrementing its timestamp.
                // Indicate that the list will need to be sorted again.
                //
                if (LastTimeStamp == CurrentTimeStamp) {
                    pEvent->SetActualTime (LastTimeStamp + increment);
                    increment++;
                    SortNeeded = true;
                }//end if we have identical timestamps

                //=====================
                // If this event's timestamp is different from the previous event,
                // reset the last timestamp and the increment value.
                //
                else {
                    LastTimeStamp = CurrentTimeStamp;
                    increment = 1.0;
                }//end if timestamps were different.

            }//end if event is enabled and has a legal timestamp

        }//end for each event in the event list
    }//end while sorting is still needed

}//end Update()

//**************************************************************************************************
//  FinishUpdate () -- Sequence Update Completion for BasicSequence Objects
//**************************************************************************************************
//! @par Description:
//!   This routine is called from within an EPICS "Callback" task, which runs at a priority slightly
//!   higher than record processing.  It does the final processing for a BasicSequence object
//!   update operation, including clearing all the "value changed" flags and performing all pending
//!   asynchronous record completions.
//!
//! @par Function:
//!   - Clear the SequenceBusy flag.
//!   - Set the effective update mode to "On Trigger".  This will permit all changes that occured
//!     while the Sequence object was updating to be processed at once with the next update.
//!   - Perform asynchronous record completion (if needed) for this Sequence's "Max Time"
//!     record.
//!   - Perform Sequence update completion for each of the sequence events associated with
//!     this Sequence.
//!   - Perform asynchronous record completion (if needed) for this Sequence's "Update Mode"
//!     record.
//!   - Perform asynchronous record completion (if needed) for this Sequence's "Update Trigger"
//!     record.
//!   - If the state of the Sequence and/or its Sequence Events changed while the Sequence was
//!     updating, if the update mode is "Immediate", start another update.
//!
//! @par Member Variables Referenced:
//! - \e  EffectiveUpdateMode = (modified) Set to "On Trigger" during asynchronous record completion
//! - \e  EventList           = (input)    Array of sequence events belonging to this Sequence
//! - \e  NumEvents           = (input)    Number of sequence events in the EventList array
//! - \e  SequenceBusy        = (modified) Set to false
//! - \e  UpdateMode          = (modified) Desired update mode
//! - \e  UpdateModePending   = (modified) True if "Update Mode" record needs asynch completion
//! - \e  UpdateModeRecord    = (input)    Address of the "Update Mode" record (if present)
//! - \e  UpdateStartPending  = (modified) True if a new update is pending
//! - \e  UpdateTriggerRecord = (input)    Address of the "Update Trigger" record (if present)
//!
//**************************************************************************************************

void
BasicSequence::FinishUpdate ()
{
    //=====================
    // Local variables
    //
    rset*  pRset;                       // Pointer to record support entry table
    bool   reprocess = false;           // True if pending updates require we do another update

    //=====================
    // o Lock access to the Sequence object.
    // o Change the Sequence update state to "Not Updating".
    // o Set the effective update mode to "On Trigger" so that any changes that occur
    //   as a result of asynchronous record completion can be processed with a single update.
    // o Inhibit updates from starting until we finish this process.
    //
    lock();
    SequenceBusy = false;
    EffectiveUpdateMode = SeqUpdateMode_OnTrigger;
    UpdatesLocked = true;

    //=====================
    // If there is a "Maximum TimeStamp" record, perform asynchronous completion on it.
    //
    if ((NULL != MaxTimeStampRecord) && MaxTimeStampPending) {
        pRset = MaxTimeStampRecord->rset;
        dbScanLock (UpdateModeRecord);
        MaxTimeStampPending = false;
        (*((REC_PROC_FN)pRset->process)) (MaxTimeStampRecord);
        dbScanUnlock (MaxTimeStampRecord);
    }//end if asynchronous completion is pending on the "Update Mode" record

    //=====================
    // Perform update completion on each of the sequence events
    // Set "reprocess" true if any of the sequence events changed state
    // after asynchronous completion.
    //
    for (epicsInt32 i = 0;  i < NumEvents;  i++) {
        if (EventList[i]->FinishUpdate())
            reprocess = true;
    }//end for each sequence event

    //=====================
    // If there is an "Update Mode" record, perform asynchronous completion on it.
    //
    if ((NULL != UpdateModeRecord) && UpdateModePending) {
        pRset = UpdateModeRecord->rset;
        dbScanLock (UpdateModeRecord);
        UpdateModePending = false;
        (*((REC_PROC_FN)pRset->process)) (UpdateModeRecord);
        dbScanUnlock (UpdateModeRecord);
    }//end if asynchronous completion is pending on the "Update Mode" record

    //=====================
    // If there is an "Update Trigger" record, perform asynchronous completion on it
    // even if it didn't start the update.
    //
    if (NULL != UpdateModeRecord) {
        pRset = UpdateTriggerRecord->rset;
        dbScanLock (UpdateTriggerRecord);
        UpdateTriggerRecord->pact = true;
        (*((REC_PROC_FN)pRset->process)) (UpdateTriggerRecord);
        dbScanUnlock (UpdateTriggerRecord);
    }//end if asynchronous completion is pending on the "Update Trigger" record

    //=====================
    // Restore the original (or possibly recently modified) update mode.
    //
    EffectiveUpdateMode = UpdateMode;

    //=====================
    // Allow update starts again.
    // Start another update if the results from the asynchronous callbacks
    // indicate we should.
    //
    UpdatesLocked = false;
    if (UpdateStartPending || (
        reprocess          &&
        !SequenceBusy      &&
        (SeqUpdateMode_Immediate == UpdateMode))) {

        StartUpdate();

    }//end if we should start another update

    //=====================
    // Sequence update has completed.
    // Unlock the Sequence object
    //
    unlock();

}//end FinishUpdate()

/**************************************************************************************************/
/*                         Public Functions Specific to the BasicSequence Class                   */
/*                                                                                                */

//**************************************************************************************************
//  DeclareEvent () -- Declare The Existence Of A Basic Sequence Event
//**************************************************************************************************
//! @par Description:
//!   Declare the existence of a basic sequence event and return a pointer to its
//!   BasicSequenceEvent object.
//!
//! @par Function:
//!   First check to see if the named event is already in this sequence.  If so, just return a
//!   pointer to the BasicSequenceEvent object.  If the event was not found, create a new
//!   BasicSequenceEvent object, add it to the sequence, and return a pointer to its object.
//!
//! @param      Name  = (input) Name of the event to create
//!
//! @return     Returns a pointer to the named BasicSequenceEvent object.
//!
//! @throw      runtime_error is thrown if the event was not in the list and we could not
//!             create a new event.
//!
//! @par Member Variables Referenced:
//! - \e        NumEvents = (modified) Number of events in this sequence
//! - \e        EventList = (modified) List of BasicSequenceEvent objects in this sequence
//!
//**************************************************************************************************

BasicSequenceEvent*
BasicSequence::DeclareEvent (const std::string &Name)
{
    //=====================
    // If the event is already in the event list, just return a pointer to its object
    //
    BasicSequenceEvent  *Event = GetEvent(Name);
    if (NULL != Event) return Event;

    //=====================
    // If the event is not already in the list, see if there is room to create one.
    //
    if (NumEvents > pEvg->GetSeqRamMaxIndex())
        throw std::runtime_error("Sequence is full");

    //=====================
    // If there is still room, try and create a new BasicSequenceEvent object
    //
    try {Event = new BasicSequenceEvent (Name, this);}
    catch (std::exception& e) {
        throw std::runtime_error(
              std::string("Unable to create new BasicSequenceEvent object - ") + e.what());
    }//end if could not create a new BasicSequenceEvent object

    //=====================
    // Add the new BasicSequenceEvent to the list and return a pointer to it.
    //
    EventList[NumEvents++] = Event;
    return Event;

}//end DeclareEvent()

//**************************************************************************************************
//  GetEvent () -- Retrieve An Event By Name
//**************************************************************************************************
//! @par Description:
//!   Retrieve an event by name
//!
//! @par Function:
//!   Searches the sequence's event table for an event whose name matches the name in the input
//!   parameter.  Returns a pointer to the BasicSequenceEvent object if it is found.
//!
//! @param      Name  = (input) Name of the event to retrieve
//!
//! @return     Returns a pointer to the named BasicSequenceEvent object.<br>
//!             Returns NULL if the requested object was not found
//!
//! @par Member Variables Referenced:
//! - \e        NumEvents = (input) Number of events in this sequence
//! - \e        EventList = (input) List of BasicSequenceEvent objects in this sequence
//!
//**************************************************************************************************

BasicSequenceEvent*
BasicSequence::GetEvent (const std::string &Name)
{
    //=====================
    // Loop to look for the requested event
    //
    for (int i = 0;  i < NumEvents;  i++) {
        if (Name == EventList[i]->GetName())
            return EventList[i];
    }//end for each event in this sequence

    //=====================
    // Return NULL if the event was not found
    //
    return NULL;

}//end GetEvent()

//**************************************************************************************************
//  UpdateNotify () -- Notify the Sequence of an Update
//**************************************************************************************************
//! @par Description:
//!   Notify the BasicSequence object that a change has occurred in one or more of the
//!   Sequence Events.
//!
//! @par Function:
//! - If the Sequence is busy with a previous update, return the 'SeqStat_Busy' status code.
//! - If the Sequence is not busy, and the effective update mode is "On Trigger", return
//!   the 'SeqStat_Idle' status code.
//! - If the Sequence is not busy, and the effective update mode is "Immediate", invoke the
//!   \e StartUpdate function and return its status code.
//!
//! @return  Returns \b SeqStat_Start if a Sequence update was started.<br>
//!          Returns \b SeqStat_Idle  if a Sequence  update was not started because the update
//!                                   mode is "On Trigger".<br>
//!          Returns \b SeqStat_Busy  if the Sequence was already updating.<br>
//!          Returns \b SeqStat_Error if the update mode could not be determined.
//!
//! @par Member Variables Referenced:
//! - \e     EffectiveUpdateMode = (input)    Indicates the Sequence object's "Effective" update
//!                                           mode (which may be different than the update mode
//!                                           specified in the "Update Mode" record).
//! - \e     SequenceBusy        = (modified) Set to "true" to indicate the Sequence is updating.
//!
//! @note
//! - This routine should only be called when the Sequence object is locked.   
//!
//**************************************************************************************************

SequenceStatus
BasicSequence::UpdateNotify()
{
    //=====================
    // Sequence is still executing a previous update
    //
    if (SequenceBusy)
        return SeqStat_Busy;

    //=====================
    // Switch on the effective update mode
    //
    switch (EffectiveUpdateMode) {

        //=====================
        // Effective Update Mode is "On Trigger"
        // Sequence update state remains "Idle" until triggered
        //
        case SeqUpdateMode_OnTrigger:
            return SeqStat_Idle;

        //=====================
        // Efective Update Mode is "Immediate"
        // Queue the update request to the EVG's Sequence update task
        //
        case SeqUpdateMode_Immediate:
            return StartUpdate();

        //=====================
        // Effective Update Mode is unknown
        // Return an error status
        //
        default:
            return SeqStat_Error;

    }//end switch on update mode

}//end UpdateNotify()

/**************************************************************************************************/
/*                           BasicSequence Private Member Functions                               */
/*                                                                                                */

//**************************************************************************************************
//  ResetTimeStamps () -- Reset The Actual Event Timestamps To Their Requested Values
//**************************************************************************************************
//! @par Description:
//!   Reset the actual timestamp of each event in this sequence back to its requested value.
//!
//! @par Member Variables Referenced:
//! - \e EventList = (input) List of BasicSequenceEvent objects in this sequence
//! - \e NumEvents = (input) Number of events in this sequence
//!
//**************************************************************************************************

void
BasicSequence::ResetTimeStamps ()
{

    //=====================
    // Loop to reset the timestamp for each event in this sequence
    //
    for (epicsInt32 i = 0;  i < NumEvents;  i++) {
        BasicSequenceEvent* pEvent = EventList[i];
        pEvent->SetActualTime (pEvent->GetEventTime());
    }//end for each event in our event list

}//end ResetTimeStamps()

/**************************************************************************************************/
/*                             BasicSequence Non-Member Functions                                 */
/*                                                                                                */


/**************************************************************************************************
|* Sort () -- Sort the Sequence Event List
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    Sort the sequence event list in ascending order based on timestamp and priority.
|*    We use a simple "Bubble-Sort" variant since the event list is expected to be
|*    mostly ordered once it is initialized.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    Sort (EventArray, Size);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    Size        = (epicsInt32)          Number of elements in the event list.
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFIED PARAMETERS:
|*    EventArray  = (BasicSequenceEvent*) Array of pointers to this sequence's sequence event
|*                                        objects.
|*
\**************************************************************************************************/

static void
Sort (BasicSequenceEvent* EventArray[], const epicsInt32 Size) {

    //=====================
    // Local Variables
    //
    epicsInt32           i, j;          // Local index variables
    BasicSequenceEvent*  temp;          // Temorary storage for swapping

    //=====================
    // Loop to examine each element and put it in its proper place
    // Invariant:  The list is sorted between the first element and the i'th element.
    //
    for (i=0;  i < (Size-1);  i++) {
        j = i + 1;

        //=====================
        // An element is out of place, if its value is less than the element before it.
        // Swap the element with its predecessor, and then sift it up to its proper place
        // in the list.
        //
        if (*(EventArray[j]) < *(EventArray[i])) {
            temp = EventArray[i];
            EventArray[i] = EventArray[j];
            EventArray[j] = temp;
            SiftUp (EventArray, i);
        }//end if element j is in the wrong order

    }//end for each element in the array

}//end Sort();

/**************************************************************************************************
|* SiftUp () -- Move a Sequence Event To Its Proper Place In The List
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    Move the out-of-place element up the list until we find an element with a smaller value.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    SiftUp (EventArray, Start);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    Size        = (epicsInt32)          Index of the out-of-place element
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFIED PARAMETERS:
|*    EventArray  = (BasicSequenceEvent*) Array of pointers to this sequence's sequence event
|*                                        objects.
|*
\**************************************************************************************************/


static void
SiftUp (BasicSequenceEvent* EventArray[], const epicsInt32 Start) {

    //=====================
    // Local Variables
    //
    epicsInt32           i, j;          // Local index variables
    BasicSequenceEvent*  temp;          // Temorary storage for swapping

    //=====================
    // Loop to sift the out-of-place element up to its proper location:
    // Invariant:  The index "j" always points to the out-of-place element.
    //
    for (j = Start;  j > 0;  j--) {
        i = j - 1;

        //=====================
        // If element "j" is less than element "i", then element "j" is still out of order
        //
        if (*(EventArray[j]) < *(EventArray[i])) {
            temp = EventArray[i];
            EventArray[i] = EventArray[j];
            EventArray[j] = temp;
        }//end if element j is in the wrong order

        //=====================
        // If element "j" is not less than element "i", then quit
        // (we have found its proper location in the list)
        //
        else return;

    }//end for each element above the starting element

}//end SiftUp()


//!
//! @}
//end group Sequencer
