/**************************************************************************************************
|* $(MRF)/evgApp/src/SequenceRamHw.cpp -- Event Generator Hardware Sequence RAM Class
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     12 February 2010
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 12 Feb 2010  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*   This module contains the implementation of the SequenceRamHw class
|*
|*   The SequenceRamHw class provides an interface to an event generator's sequence RAMs and
|*   is used when the sequence RAMs are to be run independantly.  Use the SequenceRamShared class
|*   when a single sequence is to be shared between two or more sequence RAMS.
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
/*  SequenceRamHw Class Description                                                               */
/**************************************************************************************************/

//==================================================================================================
//! @addtogroup SequenceRAM
//! @{
//!
//==================================================================================================
//! @class      SequenceRamHw
//! @brief      Event Generator Hardware Sequence Ram Class.
//!
//! @par Description:
//!    A \b SequenceRamHw object represents a list of events and timestamps.  The timestamp
//!    determines when the event should be delivered relative to the start of the sequence.
//!
//!    Although sequences are associated with event generator cards, A sequence is an abstract
//!    concept and does not in itself reference any hardware.   An event generator may "posses"
//!    mulitple sequences.  Each SequenceRamHw must have a unique ID number.  The ID number is
//!    assigned when the SequenceRamHw object is created. A SequenceRamHw object only becomes
//!    active when it is assigned to a "Sequence RAM". The same SequenceRamHw object may not be
//!    assigned to more than one Sequence RAM.
//!
//!    SequenceRamHws are useful for machines with single (or relatively few) timelines that
//!    have no relationships between the individual events.
//!
//==================================================================================================

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include  <stdexcept>           // Standard C++ exception class
#include  <string>              // Standard C++ string class

#include  <epicsTypes.h>        // EPICS Architecture-independent type definitions

#include  <mrfCommon.h>         // MRF Common definitions
#include  <evg/evg.h>           // MRF Event generator base class definition
#include  <evg/SequenceRAM.h>   // MRF Sequence RAM base class definition
#include  <SequenceRamHw.h>     // MRF SequenceRamHw class definition


