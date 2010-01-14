/**************************************************************************************************
|* $(MRF)/evgApp/src/evg/Sequence.h -- Event Generator Sequence Base Class Definition
|*
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     13 January 2010
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 13 Jan 2010  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*   This header file contains the virtual function definitions required to implement an
|*   MRF event generator sequence class.
|*
|*--------------------------------------------------------------------------------------------------
|* HARDWARE SUPPORTED:
|*   Series 2xx Event Generator Cards
|*
|*--------------------------------------------------------------------------------------------------
|* OPERATING SYSTEMS SUPPORTED:
|*   All
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

#ifndef EVG_SEQUENCE_H_INC
#define EVG_SEQUENCE_H_INC

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <epicsTypes.h>         // EPICS Architecture-independent type definitions


/**************************************************************************************************/
/*                                  Sequence Class Definition                                     */
/*                                                                                                */

class Sequence
{

/**************************************************************************************************/
/*  Public Methods                                                                                */
/**************************************************************************************************/

public:

    //=====================
    // Getter Routines
    //
    virtual epicsInt32     getCardNum        () const = 0; // Return the logical card number
    virtual epicsInt32     getSeqNum         () const = 0; // Return sequence number
    virtual epicsInt32     getNumEvents      () const = 0; // Return number of events in sequence
    virtual epicsInt32    *getEventArray     () const = 0; // Return ptr to array of event numbers
    virtual epicsFloat64  *getTimestampArray () const = 0; // Return ptr to array of timestamps

    //=====================
    // Class Destructor
    //
    virtual ~Sequence () = 0;

};// end class Sequence //

#endif // EVG_SEQUENCE_H_INC //
