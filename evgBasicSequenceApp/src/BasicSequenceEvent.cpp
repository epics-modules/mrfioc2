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

#include <epicsTypes.h>         // EPICS Architecture-independent type definitions
#include <dbCommon.h>           // EPICS Common record structure definitions

#include <mrfCommon.h>          // MRF Common definitions

#include <BasicSequence.h>      // Basic Sequence class definition
#include <BasicSequenceEvent.h> // Basic Sequence event class definition

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
//  SetEventTime () -- Set or Change This Event's Timestamp
//**************************************************************************************************
//! @par Description:
//!   Changes the requested timestamp for this event and notifies the Basic Sequence object
//!
//! @par Function:
//!   Set the new requested timestamp.
//!   Signal the Basic Sequence object that the timestamp has changed.
//!
//! @param      Ticks  = (input) New requested timestamp expressed in "Event Clock Ticks"
//!
//! @par Member Variables Referenced:
//! - \e        RequestedTime = (modified) The requested timestamp for this event.
//!
//**************************************************************************************************

void
BasicSequenceEvent::SetEventTime (epicsFloat64 Ticks)
{
    RequestedTime = Ticks;
}

//**************************************************************************************************
//  RegisterTimeRecord () -- Register The Existence Of A BasicSequenceEvent Timestamp Record
//**************************************************************************************************
//! @par Description:
//!   Declare the existence of a BasicSequenceEvent timestamp record
//!
//! @par Function:
//!   First check to see if a timestamp record has already been registered for this event.
//!   If so, throw a runtime_error containing the name of the previously registered record.
//!   If this is the first record to register, make it the timestamp record.
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
    // Make this the timestamp record if it was the first to be declared
    //
    if (NULL == TimeRecord)
        TimeRecord = pRec;

    //=====================
    // Throw an error if we already have a timestamp record
    //
    else throw std::runtime_error (
        std::string (pSequence->GetSeqID()) +
        std::string (": Duplicate \"Event Time\" records declared for Event '") + EventName +
        std::string ("'\nPreviously declared record is: ") + TimeRecord->name);

}//end RegisterTimeRecord()

//!
//! @}
//end group Sequencer
