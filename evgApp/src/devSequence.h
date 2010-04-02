/**************************************************************************************************
|* $(MRF)/evgApp/src/devSequence.h -- EPICS Driver Support for EVG Sequencers
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     3 March 2010
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 03 Mar 2010  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*    This module provides definitions and function prototypes for the MRF event generator
|*    sequence device support module.
|*
|*    An event generator sequence is an abstract object that has no hardware implementation.
|*    Its purpose is to provide the event and timestamp lists used by the EVG sequence RAM
|*    objects.
|*
|*-------------------------------------------------------------------------------------------------
|* HARDWARE SUPPORTED:
|*   Series 2xx Event Generator Cards
|*
|*-------------------------------------------------------------------------------------------------
|* OPERATING SYSTEMS SUPPORTED:
|*   All
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

#ifndef EVG_DEV_SEQUENCE_INC
#define EVG_DEV_SEQUENCE_INC

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include  <epicsTypes.h>        // EPICS Architecture-independent type definitions
#include  <devSup.h>            // EPICS Device support messages and definitions

#include  <aoRecord.h>          // EPICS Analog output record definition
#include  <boRecord.h>          // EPICS Binary output record definition
#include  <mbboRecord.h>        // EPICS Multi-Bit Binary output record definition

#include  <evg/Sequence.h>      // MRF Sequence Base Class


/**************************************************************************************************/
/*  Type Definitions                                                                              */
/**************************************************************************************************/

//=====================
// Function Type for Declaring a Sequence Object to the Event Generator
//
typedef Sequence* (*SEQ_DECLARE_FUN) (epicsInt32 Card, epicsInt32 SeqNum);


/**************************************************************************************************/
/*  Augmented Device Support Entry Table Definitions for Sequence Records                         */
/**************************************************************************************************/

/*=====================
 * Augmented Device Support Entry Table (DSET) for analog input and analog output records
 */
#define DSET_SEQ_ANALOG_NUM  7     // Number of entries in an augmented analog I/O DSET
struct SeqAnalogDSET {
    long                number;	         // Number of support routines
    DEVSUPFUN           report;	         // Report routine
    DEVSUPFUN           init;	         // Device suppport initialization routine
    DEVSUPFUN           init_record;     // Record initialization routine
    DEVSUPFUN           get_ioint_info;  // Get io interrupt information
    DEVSUPFUN           perform_io;      // Read or Write routine
    DEVSUPFUN           special_linconv; // Special linear-conversion routine
    SEQ_DECLARE_FUN     declare_sequence;// Sequence declaration routine
};/*end SeqAnalogDSET*/

//=====================
// Augmented Device Support Entry Table (DSET) for binary input and binary output records
//
#define DSET_SEQ_BINARY_NUM  6     // Number of entries in an augmented binary I/O DSET
struct SeqBinaryDSET {
    long	        number;	         // Number of support routines
    DEVSUPFUN	        report;		 // Report routine
    DEVSUPFUN	        init;	         // Device suppport initialization routine
    DEVSUPFUN	        init_record;     // Record initialization routine
    DEVSUPFUN	        get_ioint_info;  // Get io interrupt information
    DEVSUPFUN           perform_io;      // Read or Write routine
    SEQ_DECLARE_FUN     declare_sequence;// Sequence declaration routine
};//end SeqBinaryDSET


//=====================
// Augmented Device Support Entry Table (DSET) for Multi-Bit Binary Input and Output Records
//
#define DSET_SEQ_MBB_NUM     6     // Number of entries in an augmented multi-bit binary I/O DSET
struct SeqMbbDSET {
    long	        number;	         // Number of support routines
    DEVSUPFUN	        report;		 // Report routine
    DEVSUPFUN	        init;	         // Device suppport initialization routine
    DEVSUPFUN	        init_record;     // Record initialization routine
    DEVSUPFUN	        get_ioint_info;  // Get io interrupt information
    DEVSUPFUN           perform_io;      // Read or Write routine
    SEQ_DECLARE_FUN     declare_sequence;// Sequence declaration routine
};//end SeqMbbDSET

/**************************************************************************************************/
/*  Prototypes for Exported Functions                                                             */
/**************************************************************************************************/

//=====================
// Analog Output Device Support Routines
//
epicsStatus EgSeqAoInitRecord   (aoRecord *pRec);
epicsStatus EgSeqAoWrite        (aoRecord *pRec);
epicsStatus EgSeqAoLinConv      (aoRecord *pRec, bool AfterChange);

//=====================
// Binary Output Device Support Routines
//
epicsStatus EgSeqBoInitRecord   (boRecord *pRec);
epicsStatus EgSeqBoWrite        (boRecord *pRec);


//=====================
// Multi-Bit Binary Output Device Support Routines
//
epicsStatus EgSeqMbboInitRecord (mbboRecord *pRec);
epicsStatus EgSeqMbboWrite      (mbboRecord *pRec);

#endif // EVG_DEV_SEQUENCE_INC //
