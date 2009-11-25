/**************************************************************************************************
|* $(TIMING)/evgApp/src/Sequence.cpp -- Event Generator Sequence Class
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
|*    This module contains the implementation of the Sequence class
|*
|*-------------------------------------------------------------------------------------------------
|* HARDWARE SUPPORTED:
|*   Series 2xx Event Generator Cards
|*     Modular Register Mask
|*     APS Register Mask
|*
|*-------------------------------------------------------------------------------------------------
|* OPERATING SYSTEMS SUPPORTED:
|*   vxWorks
|*   RTEMS
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
/*  Sequence Class Description                                                                    */
/**************************************************************************************************/

//==================================================================================================
//! @addtogroup Sequencer
//! @{
//!
//==================================================================================================
//! @class      Sequence
//! @brief      Event Generator Sequence Class.
//!
//! @par Description:
//!    A \b Sequence object represents a list of events and timestamps.  The timestamp determines
//!    when the event should be delivered relative to the start of the sequence.  A system may
//!    have mulitple sequences.  Each sequence must have a unique ID number. The ID number is
//!    assigned when the Sequence object is created. A Sequence object only becomes active when
//!    it is assigned to a "Sequence RAM". The same Sequence object may not be assigned to
//!    more than one Sequence RAM.
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

#include <Sequence.h>           // Sequence class definition
#include <SequenceEvent.h>      // SequenceEvent class definition

/**************************************************************************************************/
/*                                  Sequence Class Definition                                     */
/*                                                                                                */


//**************************************************************************************************
//  Sequence () -- Class Constructor
//**************************************************************************************************
//! @par Description:
//!   Class Constructor
//!
//! @par Function:
//!   Create an event generator Sequence object and assign it to the specified sequence ID number
//!
//! @param Number  = (input) Sequence ID number.
//!                  The sequence ID number must be unique for every Sequence object in the system.
//!
//**************************************************************************************************

Sequence::Sequence (epicsInt32  Number):
    SequenceNumber(Number),
    NumEvents(0)
{
    //=====================
    // Convert the sequence ID number to a string
    //
    sprintf (SeqNumString, "%d", Number);

    //=====================
    // Initialize the event array
    //
    for (int i = 0;  i < MRF_MAX_SEQUENCE_EVENTS;  i++)
        EventList[i] = NULL;

}//end Constructor

//**************************************************************************************************
//  DeclareEvent () -- Declare The Existence Of A Sequence Event
//**************************************************************************************************
//! @par Description:
//!   Declare the existence of a sequence event and return a pointer to its SequenceEvent object.
//!
//! @par Function:
//!   First check to see if the named event is already in this sequence.  If so, just return a
//!   pointer to the SequenceEvent object.  If the event was not found, create a new SequenceEvent
//!   object, add it to the sequence, and return a pointer to its object.
//!
//! @param      Name  = (input) Name of the event to create
//!
//! @return     Returns a pointer to the named SequenceEvent object.
//!
//! @throw      runtime_error is thrown if the event was not in the list and we could not
//!             create a new event.
//!
//! @par Member Variables Referenced:
//! - \e        NumEvents = (modified) Number of events in this sequence
//! - \e        EventList = (modified) List of SequenceEvent objects in this sequence
//!
//**************************************************************************************************

SequenceEvent*
Sequence::DeclareEvent (const std::string &Name)
{
    //=====================
    // If the event is already in the event list, just return a pointer to its object
    //
    SequenceEvent  *Event = GetEvent(Name);
    if (NULL != Event) return Event;

    //=====================
    // If the event is not already in the list, see if there is room to create one.
    //
    if (NumEvents >= MRF_MAX_SEQUENCE_EVENTS)
        throw std::runtime_error("Sequence is full");

    //=====================
    // If there is still room, try and create a new SequenceEvent object
    //
    try {Event = new SequenceEvent (Name, this);}
    catch (std::exception& e) {
        throw std::runtime_error(
              std::string("Unable to create new SequenceEvent object - ") + e.what());
    }//end if could not create a new SequenceEvent object

    //=====================
    // Add the new SequenceEvent to the list and return a pointer to it.
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
//!   parameter.  Returns a pointer to the SequenceEvent object if it is found.
//!
//! @param      Name  = (input) Name of the event to retrieve
//!
//! @return     Returns a pointer to the named SequenceEvent object.<br>
//!             Returns NULL if the requested object was not found
//!
//! @par Member Variables Referenced:
//! - \e        NumEvents = (input) Number of events in this sequence
//! - \e        EventList = (input) List of SequenceEvent objects in this sequence
//!
//**************************************************************************************************

SequenceEvent*
Sequence::GetEvent (const std::string &Name)
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
