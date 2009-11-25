/**************************************************************************************************
|* $(TIMING)/evgApp/src/devSequence.cpp -- EPICS Device Support for EVG Sequences
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     23 November 2009
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 23 Nov 2009  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*    This module contains EPICS device support for event generator Sequence objects.
|*    It also contains some global utility routines used by Sequence, SequenceEvent, and
|*    SequenceRAM objects.
|*
|*-------------------------------------------------------------------------------------------------
|* HARDWARE SUPPORTED:
|*   Series 2xx Event Generator Cards
|*     Modular Register Mask
|*
|*-------------------------------------------------------------------------------------------------
|* OPERATING SYSTEMS SUPPORTED:
|*   vxWorks
|*   RTEMS
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|*  o This module does not support the APS-style register map because it relies on interrupts
|*    from the event generator card.  EVG interrupts are not supported in the APS register map.
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

//==================================================================================================
//  Sequencer Group Definition
//==================================================================================================
//! @defgroup   Sequencer Event Generator Sequence Control Library
//! @brief      Defines how the event sequencer works.
//!
//! A "Sequence" is an ordered list of event codes and timestamps.
//!
//! There may be any number of sequences defined for a given event generator card, even though
//! an event generator card only has two sequence RAMs.
//! 
//! @{
//!
//==================================================================================================

//==================================================================================================
//  devSequence File Description
//==================================================================================================
//! @file       devSequence.cpp
//! @brief      EPICS Device Support for Event Generator Sequencers.
//!
//! @par Description:
//!    This file provides EPICS Device support for event generator Sequence objects.
//!
//! @note
//!    This module does not support the APS-style register map because it relies on interrupts
//!    from the event generator module to determine when it is safe to update the sequence RAMs.
//!    The APS-style register map does not support event generator interrupts.
//!
//==================================================================================================

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <stdexcept>            // Standard C++ exception definitions
#include <map>                  // Standard C++ map template

#include <epicsTypes.h>         // EPICS Architecture-independent type definitions
#include <devSup.h>             // EPICS Device support definitions
#include <initHooks.h>          // EPICS IOC Initialization hooks support library
#include <iocsh.h>              // EPICS IOC shell support library
#include <registryFunction.h>   // EPICS Registry support library

#include  <Sequence.h>          // MRF Sequence Class
#include  <SequenceEvent.h>     // MRF Sequence Event Class
#include  <devSequence.h>       // MRF Sequence device support declarations
#include  <drvEvg.h>            // MRF Event Generator driver infrastructure routines

#include <epicsExport.h>        // EPICS Symbol exporting macro definitions

/**************************************************************************************************/
/*  Type Definitions                                                                              */
/**************************************************************************************************/

//=====================
// SequenceList:  Associates sequence numbers with Sequence objects
//
typedef std::map <epicsInt32, Sequence*>      SequenceList;

//=====================
// CardList:  Associates card numbers with sequence lists
//
typedef std::map <epicsInt32, SequenceList*>  CardList;

/**************************************************************************************************/
/*  Variables Global To This Module                                                               */
/**************************************************************************************************/

//=====================
// List of sequence objects for each event generator card
//
static
CardList   CardSequences;

//**************************************************************************************************
//  EgDeclareSequence () -- Declare The Existence Of A Sequence
//**************************************************************************************************
//! @par Description:
//!   Declare the existence of a sequence and return a pointer to its Sequence object.
//!
//! @par Function:
//!   First check to see if the requested sequence is already in our list.  If so, just return a
//!   pointer to the Sequenc object.  If the sequence was not found, create a new Sequence
//!   object, add it to the list, and return a pointer to its object.
//!
//! @param      CardNum  = (input) Card number of the event generator this sequence belongs to.
//! @param      SeqNum   = (input) Sequence ID number of the sequence to declare
//!
//! @return     Returns a pointer to the Sequence object.
//!
//! @throw      runtime_error is thrown if we needed to create a new Sequence object but couldn't.
//!
//! @par External Data Referenced:
//! - \e        CardSequences = (modified) List of sequences for each EVG card.
//!
//**************************************************************************************************

Sequence*
EgDeclareSequence (epicsInt32 CardNum, epicsInt32 SeqNum)
{
    //=====================
    // Local variables
    //
    SequenceList*   List;       // Reference to the sequence list for the specified EVG card
    EVG*            pEvg;       // Pointer to the associated event generator object
    Sequence*       pSeq;       // Pointer to the desired Sequence object

    //=====================
    // Abort if the EVG card has not been initialized
    //
    if (NULL == (pEvg = EgGetCard(CardNum)))
        throw std::runtime_error("Event Generator card not configured");

    //=====================
    // Get the sequence list for this card
    //
    CardList::iterator card = CardSequences.find(CardNum);
    if (card != CardSequences.end())
        List = card->second;

    //=====================
    // If we don't have a sequence list for this EVG card, create one
    //
    else {
        try {List = new SequenceList();}

       /* Abort if we could not create the sequence list */
        catch (std::exception& e) {
            throw std::runtime_error(std::string("Unable to create sequence list - ") + e.what());
        }//end if we could not create a sequence list

       /* Add the sequence list to this card */
        CardSequences[CardNum] = List;

    }//end if we didn't already have a sequence list for this card

    //=====================
    // Get the desired Sequence object
    // 
    SequenceList::iterator index = List->find(SeqNum);
    if (index != List->end())
        pSeq = index->second;

    //=====================
    // If we don't have a Sequence object for this ID, create one
    //
    else {
        try {pSeq = new Sequence(SeqNum);}

       /* Abort if we could not create the Sequence object */
        catch (std::exception& e) {
            throw std::runtime_error(
                  std::string("Unable to create new Sequence object - ") + e.what());
        }//end if could not create a new Sequence object

       /* Add the Sequence object to this list */
        (*List)[SeqNum] = pSeq;

    }//end if we didn't already have a Sequence object

    //=====================
    // Return the pointer to the desired Sequence object
    //
    return pSeq;

}//end EgDeclareSequence()

//**************************************************************************************************
//  EgGetSequence () -- Retrieve A Sequence Object
//**************************************************************************************************
//! @par Description:
//!   Retrieve the requested sequence object
//!
//! @par Function:
//!   Searches the sequence table a sequence whose number matches the number specified in the input
//!   parameter.  Returns a pointer to the Sequence object if it is found.
//!
//! @param      CardNum  = (input) Card number of the event generator this sequence belongs to.
//! @param      SeqNum   = (input) Sequence ID number of the sequence to retrieve
//!
//! @return     Returns a pointer to the Sequence object.<br>
//!             Returns NULL if the requested object was not found
//!
//! @par External Data Referenced:
//! - \e        CardSequences = (input) List of sequences for each EVG card.
//!
//**************************************************************************************************

Sequence*
EgGetSequence (epicsInt32 CardNum, epicsInt32 SeqNum)
{
    //=====================
    // Local variables
    //
    SequenceList*   List;       // Reference to the sequence list for the specified EVG card

    //=====================
    // First, make sure the EVG card has been initialized
    //
    if (NULL == EgGetCard(CardNum))
        return NULL;

    //=====================
    // Next, see if any sequences have been defined for this card
    //
    CardList::iterator card = CardSequences.find(CardNum);
    if (card == CardSequences.end())
        return NULL;
    List = card->second;
        
    //=====================
    // Finally, look for the requested sequence in the card's sequence list
    //
    SequenceList::iterator index = List->find(SeqNum);
    if (index != List->end()) {
        return (index->second);
    }//end if the Sequence object is in the list

    //=====================
    // Return NULL if the sequence was not found
    //
    return NULL;

}//end EgGetSequence()

//!
//| @}
//end group Sequencer
