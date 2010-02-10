/**************************************************************************************************
|* $(MRF)/evgBasicSequenceApp/src/BasicSequenceEvent.cpp -- Basic Sequence Event Class
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     18 November 2009
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 18 Nov 2009  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*    This module contains the implementation of the BasicSequenceEvent class
|*
|*   The BasicSequenceEvent class uses individual EPICS records to define a sequence event's
|*   properties such as event code, timestamp, enable/disable status, and collision priority.
|*
|*   BasicSequenceEvent objects define the sequence events in a BasicSequence object.
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

/**************************************************************************************************/
/*  BasicSequenceEvent Class Description                                                          */
/**************************************************************************************************/

//==================================================================================================
//! @addtogroup Sequencer
//! @{
//!
//==================================================================================================
//! @class      BasicSequenceEvent
//! @brief      Event Generator BasicSequenceEvent Class.
//!
//! @par Description:
//!   The \b BasicSequenceEvent object represents a single event in an event generator sequence.
//!   Every event in a sequence must have a unique name associated with it -- even if the
//!   event code is duplicated.  The unique name is assigned at the time the BasicSequenceEvent
//!   object is created.
//!
//==================================================================================================


/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <stdexcept>            // Standard C++ exception definitions
#include <string>               // Standard C++ string class

#include <math.h>               // Standard C math library

#include <epicsTypes.h>         // EPICS Architecture-independent type definitions
#include <dbCommon.h>           // EPICS Common record structure definitions

#include <mrfCommon.h>          // MRF Common definitions

#include <BasicSequence.h>      // BasicSequence class definition
#include <BasicSequenceEvent.h> // BasicSequenceEvent class definition

/**************************************************************************************************/
/*                              Class Member Function Definitions                                 */
/*                                                                                                */

//**************************************************************************************************
//  BasicSequenceEvent () -- Class Constructor
//**************************************************************************************************
//! @par Description:
//!   Class Constructor
//!
//! @par Function:
//!   Creates an EventSequence object and assigns it to the name specified by the input parameter
//!
//! @param Name = (input) Name to be assigned to this event.<br>
//!               Every event assigned to a specific sequence must have a unique name -- even
//!               if the event codes are identical.
//! @param pSeq = (input) Pointer to the Basic Sequence object that this event belongs to.
//!
//**************************************************************************************************

BasicSequenceEvent::BasicSequenceEvent (const std::string& Name, BasicSequence* pSeq) :

    //=====================
    // Initialize the data members
    //
    EventName(Name),            // Set the event name
    pSequence(pSeq),            // Pointer to the Sequence object

    EventCode(0),               // Default event code is 0 (no code)
    EventCodeRecord(NULL),      // Pointer to event code record

    RequestedTime(0.0),         // Default requested time is 0 ticks
    ActualTime(0.0),            // Assigned timestamp
    TimeRecord(NULL),           // Pointer to event time record

    Enable(true),               // Default enable flag is true (event is enabled)
    EnableRecord(NULL),         // Pointer to event enable record

    Priority(0),                // Default priority is 0 (mid range)
    PriorityRecord(NULL)        // Pointer to event priority record

{};//end constructor

//**************************************************************************************************
//  SetEventCode () -- Set or Change This Event's Event Code
//**************************************************************************************************
//! @par Description:
//!   Changes the event code for this event and notifies the Basic Sequence object
//!
//! @par Function:
//!   - Set the new event code.
//!   - Signal the Basic Sequence object that the event code has changed.
//!
//! @param      Code      = (input) New requested event code
//!
//! @return     Returns ERROR if the event code was out of range.<br>
//!             Returns OK if the event code was valid.
//!
//! @par Member Variables Referenced:
//! - \e        EventCode = (modified) The event code for this event.
//!
//**************************************************************************************************

epicsStatus
BasicSequenceEvent::SetEventCode (epicsInt32 Code)
{
    //=====================
    // Abort if the event code is out of range.
    //
    if ((Code < 0) || (Code >= MRF_NUM_EVENTS))
        return ERROR;

    //=====================
    // Set the new event code
    //
    EventCode = Code;
    return OK;

}//end SetEventCode()

//**************************************************************************************************
//  SetEventEnable () -- Enable or Disable the Event
//**************************************************************************************************
//! @par Description:
//!   Enables or disables the event and notifies the Basic Sequence object
//!
//! @par Function:
//!   - Set the new enable status
//!   - Signal the Basic Sequence object that the enable status has changed.
//!
//! @param      Value       = (input) New enable state
//!
//! @return     Always returns OK
//!
//! @par Member Variables Referenced:
//! - \e        EventEnable = (modified) The enable state for this event.
//!
//**************************************************************************************************

epicsStatus
BasicSequenceEvent::SetEventEnable (bool Value)
{
    //=====================
    // Set the new enable state
    //
    Enable = Value;
    return OK;

}//end SetEventEnable()

//**************************************************************************************************
//  SetEventPriority () -- Set or Change This Event's Priority Level
//**************************************************************************************************
//! @par Description:
//!   Changes the priority for this event and notifies the Basic Sequence object
//!
//! @par Function:
//!   - Set the new event priority.
//!   - Signal the Basic Sequence object that the event's priority has changed.
//!
//! @param      Level    = (input) New requested priority level
//!
//! @return     Always returns OK.
//!
//! @par Member Variables Referenced:
//! - \e        Priority = (modified) The requested priority level for this event.
//!
//**************************************************************************************************

epicsStatus
BasicSequenceEvent::SetEventPriority (epicsInt32 Level)
{
    //=====================
    // Set the new event code
    //
    Priority = Level;
    return OK;

}//end SetEventPriority()

//**************************************************************************************************
//  SetEventTime () -- Set or Change This Event's Timestamp
//**************************************************************************************************
//! @par Description:
//!   Changes the requested timestamp for this event and notifies the Basic Sequence object
//!
//! @par Function:
//!   - Set the new requested timestamp.
//!   - Signal the Basic Sequence object that the timestamp has changed.
//!
//! @param      Ticks  = (input) New requested timestamp expressed in "Event Clock Ticks"
//!
//! @return     Always returns OK.
//!
//! @par Member Variables Referenced:
//! - \e        RequestedTime = (modified) The requested timestamp for this event.
//!
//**************************************************************************************************

epicsStatus
BasicSequenceEvent::SetEventTime (epicsFloat64 Ticks)
{
    RequestedTime = Ticks;
    ActualTime = floor(Ticks);
    return OK;

}//end SetEventTime()

//**************************************************************************************************
//  RegisterCodeRecord () -- Register The Existence Of A BasicSequenceEvent Event Code Record
//**************************************************************************************************
//! @par Description:
//!   Declare the existence of a BasicSequenceEvent event code record
//!
//! @par Function:
//!   First check to see if an event code record has already been registered for this event.
//!   If so, throw a runtime_error containing the name of the previously registered record.
//!   If this is the first record to register, make it the official event code record.
//!
//! @param   pRec  = (input) Pointer to the record we wish to register
//!
//! @throw   runtime_error is thrown if an event code record was already registered.
//!
//! @par Member Variables Referenced:
//! - \e     EventCodeRecord = (modified) Points to the event code record registered for this event.
//!
//**************************************************************************************************

void
BasicSequenceEvent::RegisterCodeRecord (dbCommon *pRec)
{
    //=====================
    // Make this the official event code record if it was the first to be declared
    //
    if (NULL == EventCodeRecord)
        EventCodeRecord = pRec;

    //=====================
    // Throw an error if we already have an event code record
    //
    else throw std::runtime_error (
        std::string (pSequence->GetSeqID()) +
        std::string (": Duplicate 'Event Code' records declared for Event '") + EventName +
        std::string ("'\nPreviously declared record is: ") + EventCodeRecord->name);

}//end RegisterCodeRecord()

//**************************************************************************************************
//  RegisterEnableRecord () -- Register The Existence Of A BasicSequenceEvent Event Enable Record
//**************************************************************************************************
//! @par Description:
//!   Declare the existence of a BasicSequenceEvent event enable record
//!
//! @par Function:
//!   First check to see if an event enable record has already been registered for this event.
//!   If so, throw a runtime_error containing the name of the previously registered record.
//!   If this is the first record to register, make it the official event enable record.
//!
//! @param   pRec         = (input) Pointer to the record we wish to register
//!
//! @throw   runtime_error is thrown if an event enable record was already registered.
//!
//! @par Member Variables Referenced:
//! - \e     EnableRecord = (modified) Points to the event enable record registered for this event.
//!
//**************************************************************************************************

void
BasicSequenceEvent::RegisterEnableRecord (dbCommon *pRec)
{
    //=====================
    // Make this the official event enable record if it was the first to be declared
    //
    if (NULL == EnableRecord)
        EnableRecord = pRec;

    //=====================
    // Throw an error if we already have an event enable record
    //
    else throw std::runtime_error (
        std::string (pSequence->GetSeqID()) +
        std::string (": Duplicate 'Enable' records declared for Event '") + EventName +
        std::string ("'\nPreviously declared record is: ") + EnableRecord->name);

}//end RegisterEnableRecord()

//**************************************************************************************************
//  RegisterPriorityRecord () -- Register The Existence Of A BasicSequenceEvent Priority Record
//**************************************************************************************************
//! @par Description:
//!   Declare the existence of a BasicSequenceEvent priority record
//!
//! @par Function:
//!   First check to see if a priority record has already been registered for this event.
//!   If so, throw a runtime_error containing the name of the previously registered record.
//!   If this is the first record to register, make it the event's official priority record.
//!
//! @param   pRec  = (input) Pointer to the record we wish to register
//!
//! @throw   runtime_error is thrown if a priority record was already registered.
//!
//! @par Member Variables Referenced:
//! - \e     PriorityRecord = (modified) Points to the priority record registered for this event.
//!
//**************************************************************************************************

void
BasicSequenceEvent::RegisterPriorityRecord (dbCommon *pRec)
{
    //=====================
    // Make this the official priority record if it was the first to be declared
    //
    if (NULL == PriorityRecord)
        PriorityRecord = pRec;

    //=====================
    // Throw an error if we already have a priority record
    //
    else throw std::runtime_error (
        std::string (pSequence->GetSeqID()) +
        std::string (": Duplicate 'Priority' records declared for Event '") + EventName +
        std::string ("'\nPreviously declared record is: ") + PriorityRecord->name);

}//end RegisterPriorityRecord()

//**************************************************************************************************
//  RegisterTimeRecord () -- Register The Existence Of A BasicSequenceEvent Timestamp Record
//**************************************************************************************************
//! @par Description:
//!   Declare the existence of a BasicSequenceEvent timestamp record
//!
//! @par Function:
//!   First check to see if a timestamp record has already been registered for this event.
//!   If so, throw a runtime_error containing the name of the previously registered record.
//!   If this is the first record to register, make it the official timestamp record.
//!
//! @param      pRec  = (input) Pointer to the record we wish to register
//!
//! @throw      runtime_error is thrown if a timestamp record was already registered.
//!
//! @par Member Variables Referenced:
//! - \e        TimeRecord = (modified) Points to the timestamp record registered for this event.
//!
//**************************************************************************************************

void
BasicSequenceEvent::RegisterTimeRecord (dbCommon *pRec)
{
    //=====================
    // Make this the official timestamp record if it was the first to be declared
    //
    if (NULL == TimeRecord)
        TimeRecord = pRec;

    //=====================
    // Throw an error if we already have a timestamp record
    //
    else throw std::runtime_error (
        std::string (pSequence->GetSeqID()) +
        std::string (": Duplicate 'Time' records declared for Event '") + EventName +
        std::string ("'\nPreviously declared record is: ") + TimeRecord->name);

}//end RegisterTimeRecord()

//**************************************************************************************************
//  SanityCheck () -- Make Sure This BasicSequenceEvent Object Has Been Properly Defined
//**************************************************************************************************
//! @par Description:
//!   Make sure that a BasicSequenceEvent object has been properly defined.
//!   To be properly defined, and BasicSequenceEvent object must have at least an 'Event Code'
//!   and a 'Time' record.  If either of these two records is missing, disable all the records
//!   for this event and return an ERROR status.
//!
//! @return   Returns OK if the event has both an event code and a timestamp<br>
//!           Returns ERROR if the event was not properly defined
//!
//! @par Member Variables Referenced:
//! - \e     TimeRecord      = (input) Points to the timestamp record registered for this event.
//! - \e     EnableRecord    = (input) Points to the enable record registered for this event.
//! - \e     PriorityRecord  = (input) Points to the priority record registered for this event.
//! - \e     EventCodeRecord = (input) Points to the event code record registered for this event.
//!
//**************************************************************************************************

epicsStatus
BasicSequenceEvent::SanityCheck () {

    //=====================
    // Local variables
    //
    epicsStatus   status = OK;

    //=====================
    // Make sure the sequence event has an Event Code record
    //
    if (NULL == EventCodeRecord) {
        status = ERROR;
        printf ("%s Event '%s', has no 'Event Code' record defined.\n",
                pSequence->GetSeqID(), EventName.c_str());
    }//if no Event Code record was defined

    //=====================
    // Make sure the sequence event has a Timestamp record
    //
    if (NULL == TimeRecord) {
        status = ERROR;
        printf ("%s Event '%s', has no 'Time' record defined.\n",
                pSequence->GetSeqID(), EventName.c_str());
    }//if no Time record was defined
        
    //=====================
    // If we failed the sanity check, disable all the sequence event's records
    //
    if (OK != status) {
        if (NULL != TimeRecord)      mrfDisableRecord ((dbCommon *)TimeRecord);
        if (NULL != EnableRecord)    mrfDisableRecord ((dbCommon *)EnableRecord);
        if (NULL != PriorityRecord)  mrfDisableRecord ((dbCommon *)PriorityRecord);
        if (NULL != EventCodeRecord) mrfDisableRecord ((dbCommon *)EventCodeRecord);
    }//end if event should be disabled

    //=====================
    // Return the sanity-status of this event
    //
    return status;

}//end SanityCheck();

void
BasicSequenceEvent::Report () const {
    printf ("    %5d   %s    %18.0f   %8d  %s\n",
            EventCode,
            Enable? "Ena" : "Dis",
            ActualTime,
            Priority,
            EventName.c_str());
    return;
}//end Report()

bool
BasicSequenceEvent::operator < (const BasicSequenceEvent &Event) const {

    //=====================
    // If the two event times are different,
    // use the actual times to determine the lesser value
    //
    if (this->ActualTime != Event.ActualTime)
        return (this->ActualTime < Event.ActualTime);

    //=====================
    // If the two event times are the same, use the "jostle priority" to arrange them
    // in order of decreasing priority.
    //
    else
        return (this->Priority > Event.Priority);

}//end operator< ()

//!
//! @}
//end group Sequencer
