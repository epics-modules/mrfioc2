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
|*    This module contains the implementation of the BasicSequence class
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
//!    Although sequences are associated with event generator cards, A sequence is an abstract
//!    concept and does not in itself reference any hardware.   An event generator may "posses"
//!    mulitple sequences.  Each BasicSequence must have a unique ID number.  The ID number is
//!    assigned when the BasicSequence object is created. A BasicSequence object only becomes
//!    active when it is assigned to a "Sequence RAM". The same BasicSequence object may not be
//!    assigned to more than one Sequence RAM.
//!
//!    BasicSequences are useful for machines with single (or relatively few) timelines that
//!    have no relationships between the individual events.
//!
//==================================================================================================

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <stdexcept>            // Standard C++ exception class
#include <string>               // Standard C++ string class

#include <stdio.h>              // Standard C I/O library

#include <epicsTypes.h>         // EPICS Architecture-independent type definitions

#include <mrfCommon.h>          // MRF Common definitions
#include  <drvEvg.h>            // MRF Event Generator driver infrastructure routines
#include <BasicSequence.h>      // MRF BasicSequence class definition
#include <BasicSequenceEvent.h> // MRF BasicSequenceEvent class definition

/**************************************************************************************************/
/*                               BasicSequence Class Definition                                   */
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
    ClassID(BASIC_SEQ_CLASS_ID),
    CardNumber(Card),
    SequenceNumber(Number),
    NumEvents(0)
{
    //=====================
    // Create the sequence ID string
    //
    sprintf (SeqID, "Card %d, Seq %d", Card, Number);

    //=====================
    // Initialize the event array
    //
    for (int i = 0;  i < MRF_MAX_SEQUENCE_EVENTS;  i++)
        EventList[i] = NULL;

    //=====================
    // Try to get the EVG object for the card we belong to
    //
    if (NULL == (pEvg = EgGetCard(Card)))
        throw std::runtime_error (std::string(SeqID) + ": Event generator card not configured");

}//end Constructor

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
    if (NumEvents >= MRF_MAX_SEQUENCE_EVENTS)
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

//!
//! @}
//end group Sequencer
