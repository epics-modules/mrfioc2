/**************************************************************************************************
|* $(MRF)/evgApp/src/SequenceRamHw.h -- Event Generator Hardware Sequence RAM Class Definition
|*
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
|*   This header file contains the definition of a SequenceRamHw class.
|*
|*   The SequenceRamHw class provides an interface to an event generator's sequence RAMs and
|*   is used when the sequence RAMs are to be run independantly.  Use the SequenceRamShared class
|*   when a single sequence is to be shared between two or more sequence RAMS.
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

#ifndef EVG_SEQUENCE_RAM_HW_INC
#define EVG_SEQUENCE_RAM_HW_INC

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <string>               // Standard C++ string class
#include <epicsTypes.h>         // EPICS Architecture-independent type definitions

#include <mrfCommon.h>          // MRF Common definitions

#include <evg/evg.h>            // Event generator base class definition
#include <evg/SequenceRAM.h>    // Event generator sequence RAM base class definition

/**************************************************************************************************/
/*                               SequencerRamHw Class Definition                                  */
/*                                                                                                */

class SequenceRamHw: public SequenceRAM
{

/**************************************************************************************************/
/*  Public Methods                                                                                */
/**************************************************************************************************/

public:

    //=====================
    // Class constructor
    //
    SequenceRamHw (EVG* pEvg, epicsInt32 Ram);

    //=====================
    // Destructor
    //
    ~SequenceRamHw ();

/**************************************************************************************************/
/*  Private Data                                                                                  */
/**************************************************************************************************/

private:

    epicsInt32           CardNumber;         // EVG card this sequence RAM belongs to.
    epicsInt32           RamNumber;          // Sequence RAM number
    EVG*                 pEvg;               // Event generator card object

    //=====================
    // Event and Timestamp Arrays
    //
    epicsInt32*          EventCodeArray;     // Array of event codes
    epicsUInt32*         TimestampArray;     // Array of time stamps


};// end class SequenceRamHw //

#endif // EVG_SEQUENCE_RAM_HW_INC //
