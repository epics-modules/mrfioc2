/**************************************************************************************************
|* $(MRF)/evgApp/src/drvSequence.cpp -- EPICS Device Support for EVG Sequences
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
|*    This module contains EPICS driver support for event generator Sequence objects.
|*
|*    An event generator sequence is an abstract object that has no hardware implementation.
|*    Its purpose is to provide the event and timestamp lists used by the EVG Sequence RAM
|*    objects.
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
//! an event generator card typically only has two sequence RAMs.
//! 
//! @{
//!
//==================================================================================================

//==================================================================================================
//  drvSequence File Description
//==================================================================================================
//! @file       drvSequence.cpp
//! @brief      EPICS Device Support for Event Generator Sequencers.
//!
//! @par Description:
//!    This file provides EPICS driver support for event generator Sequence objects.
//!
//!    Although sequences are associated with event generator cards, an event generator
//!    sequence is an abstract object that has no hardware implementation.
//!    Its purpose is to provide the event and timestamp lists used by the EVG Sequence RAM
//!    objects.
//!
//==================================================================================================

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include  <stdexcept>           // Standard C++ exception definitions
#include  <string>              // Standard C++ sring class
#include  <map>                 // Standard C++ map template

#include  <epicsTypes.h>        // EPICS Architecture-independent type definitions
#include  <devSup.h>            // EPICS Device support definitions
#include  <initHooks.h>         // EPICS IOC Initialization hooks support library
#include  <iocsh.h>             // EPICS IOC shell support library
#include  <registryFunction.h>  // EPICS Registry support library

#include  <evg/Sequence.h>      // MRF Sequence Base Class
#include  <drvSequence.h>       // MRF Sequencer driver support declarations
#include  <drvEvg.h>            // MRF Event Generator driver infrastructure routines

#include  <epicsExport.h>       // EPICS Symbol exporting macro definitions

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
//  EgAddSequence () -- Add A Sequence To The List Of Known Sequences For Each Card
//**************************************************************************************************
//! @par Description:
//!   Add a Sequence object to the list of known sequences for the EVG card it belongs to.
//!
//! @par Function:
//!  -  Check for various error conditions such as card not initilialized and card/seq pair
//!     already on list.
//!  -  Create a sequence list for the card, if one does not already exist.
//!  -  Add the Sequence object to the sequence list for its EVG card.
//!
//! @param      pSeq  = (input) Address of Sequence object to add to the list.
//!
//! @throw      runtime_error is thrown if we needed to create a new Sequence object but couldn't.
//!
//! @par External Data Referenced:
//! - \e        CardSequences = (modified) List of sequences for each EVG card.
//!
//**************************************************************************************************

void
EgAddSequence (Sequence*  pSeq)
{
    //=====================
    // Local variables
    //
    SequenceList*   List;       // Reference to the sequence list for the specified EVG card
    char            SeqId [32]; // Sequence ID string

    //=====================
    // Extract the EVG card and sequence numbers
    //
    epicsInt32 CardNum = pSeq->getCardNum();
    epicsInt32 SeqNum  = pSeq->getSeqNum();
    sprintf (SeqId, "Card %d Seq %d: ", CardNum, SeqNum);

    //=====================
    // Try to add this Sequence to the sequence list for its EVG card
    //
    try {
        //=====================
        // Abort if the EVG card has not been initialized
        //
        if (NULL == EgGetCard(CardNum))
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
                throw std::runtime_error(std::string("Can't create sequence list: ") + e.what());
            }//end if we could not create a sequence list

            /* Add the sequence list to this card */
            CardSequences[CardNum] = List;

        }//end if we didn't already have a sequence list for this card

        //=====================
        // See if we already have a sequence with this number for this card.
        // 
        SequenceList::iterator index = List->find(SeqNum);
        if (index != List->end())
            throw std::runtime_error("Sequence already exists");

        //=====================
        // Add the Sequence object to the list
        //
        (*List)[SeqNum] = pSeq;

    }//end try

    //=====================
    // Catch any errors and rethrow them with a standard message context
    //
    catch (std::exception& e) {
        throw std::runtime_error(
              std::string("Can't create sequence for ") +
              std::string(SeqId) +
              std::string(e.what()));
    }//end rethrow error

}//end EgAddSequence()

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
