/**************************************************************************************************
|* $(MRF)/evgApp/src/devSequenceRAM.cpp -- EPICS Device Support for Sequence RAMs
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
|*    This module contains EPICS device support for event generator Sequence RAM objects.
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
/*  devSequenceRAM File Description                                                               */
/**************************************************************************************************/

//==================================================================================================
//! @addtogroup SequenceRAM
//! @{
//!
//==================================================================================================
//! @file       devSequenceRAM.cpp
//! @brief      EPICS Device Suppport for Event Generator Sequence RAMs
//!
//! @par Description:
//!   This file contains EPICS device support for event generator sequence RAMs.
//!   Typically, an event generator will have two sequence RAMS which may operate independantly
//!   or be shared by a single sequence. When sequence RAMs are operating independantly, they
//!   are addressed by their sequence numbers, which are positive non-zero integers (e.g Ram 1,
//!   Ram 2, etc.) When the sequence RAMs are shared, they are addressed as "Ram 0".  Ram 0
//!   refers to a virtual sequence RAM which alternates between the two physical sequence RAMs
//!   whenever the sequence is updated.  The shared sequence RAM (Ram 0) is the safest and most
//!   responsive method for implementing live updated on a single sequence.
//!
//!   @par
//!   A sequence RAM can be represented in the EPICS data base by up to 8 records. The records are
//!   distinguished by function codes specified in their I/O link fields (see below).
//!   Sequence RAM records, may have the following function codes:
//!   - \b Enable        (bo) Enable (arm) the sequence RAM for triggering.
//!   - \b Trigger Mode  (mbbo) Specifies how a sequence RAM gets triggered once it is enabled.
//!                      A sequence RAM may be triggered by the AC zero crossing logic or
//!                      by one of the multiplexed counters.
//!   - \b Repeat Mode   (mbbo) Specifies how a sequence repeats after it is triggered.  The
//!                      options are "Normal" (repeat on every trigger), "Continuous" (repeat
//!                      immediately after completion), and "Single" (on completion, halt and
//!                      disable the sequence.
//!   - \b Load          (longout) Load the specified sequence number into the sequence RAM
//!   - \b Start         (bo) Manually trigger the sequence RAM.
//!   - \b Stop          (bo) Manually stop the sequence RAM.
//!   - \b Running       (bi) Indicates that the sequence RAM is running a sequence
//!   - \b Enabled       (bi) Indicates that the sequence RAM is enabled.
//!
//! @par Device Type Field:
//!   A sequence RAM record will have the "Device Type" field (DTYP) set to:<br>
//!      "EVG Seq RAM"
//!
//! @par Link Format:
//!   Sequence RAM records use the INST_IO I/O link format. The INP or OUT links have the
//!   following format:<br>
//!      \@C=n; Ram=m; Fn=\<function\>
//!
//!   Where:
//!   - \b C     = Logical card number for the event generator card this sequence event belongs to.
//!   - \b Ram   = Specifies which sequence RAM is being addressed
//!   - \b Fn    = The record's function (see list above)
//!
//==================================================================================================


/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include  <stdexcept>           // Standard C++ exception definitions
#include  <string>              // Standard C++ string class
#include  <cstdlib>             // Standard C library

#include  <epicsTypes.h>        // EPICS Architecture-independent type definitions
#include  <alarm.h>             // EPICS Alarm status and severity definitions
#include  <dbAccess.h>          // EPICS Database access messages and definitions
#include  <devSup.h>            // EPICS Device support messages and definitions
#include  <iocsh.h>             // EPICS IOC shell support library
#include  <link.h>              // EPICS Database link definitions
#include  <recGbl.h>            // EPICS Global record support routines

#include  <biRecord.h>          // EPICS Binary input record definition
#include  <boRecord.h>          // EPICS Binary output record definition
#include  <longoutRecord.h>     // EPICS Long output record definition
#include  <mbboRecord.h>        // EPICS Multi-bit binary output record definition

#include  <mrfCommon.h>         // MRF Common definitions
#include  <mrfIoLink.h>         // MRF I/O link field parser
#include  <drvEvg.h>            // MRF Event generator driver infrastructure definitions
#include  <evg/evg.h>           // MRF Generic event generator class definition
#include  <evg/SequenceRAM.h>   // MRF Generic sequence RAM class definition

#include  <epicsExport.h>       // EPICS Symbol exporting macro definitions


/**************************************************************************************************/
/*  Structure And Type Definitions                                                                */
/**************************************************************************************************/

//=====================
// Common I/O link parameter definitions used by all SequenceRAM records
//
static const
mrfParmNameList SeqRamParmNames = {
    "C",        // Event generator logical card number
    "Ram",      // Sequence RAM number
    "Fn"        // Record function code
};

static const
epicsInt32  SeqRamNumParms mrfParmNameListSize(SeqRamParmNames);

//=====================
// Generic Sequence RAM setter function type definition
//
typedef  epicsStatus (SequenceRAM::*SetFunction) (epicsInt32);

//=====================
// Generic Sequence RAM getter function type definition
//
typedef  epicsStatus (SequenceRAM::*GetFunction) (&epicsInt32);

//=====================
// Common device information structure used by all BasicSequenceEvent records
//
struct devInfoStruct {
    EVG*          pEvg;         // Pointer to the event generator object for this sequence RAM
    GetFunction   Get;          // Getter Function
    SetFunction   Set;          // Setter Function
};//end devInfoStruct

//=====================
// Device Support Entry Table (DSET) for binary input and output records
//
struct BinaryDSET {
    long	number;	         // Number of support routines
    DEVSUPFUN	report;		 // Report routine
    DEVSUPFUN	init;	         // Device suppport initialization routine
    DEVSUPFUN	init_record;     // Record initialization routine
    DEVSUPFUN	get_ioint_info;  // Get io interrupt information
    DEVSUPFUN   perform_io;      // Read or Write routine
};//end BinaryDSET

//=====================
// Device Support Entry Table (DSET) for long output records
//
struct LongOutDSET {
    long	number;	         // Number of support routines
    DEVSUPFUN	report;		 // Report routine
    DEVSUPFUN	init;	         // Device suppport initialization routine
    DEVSUPFUN	init_record;     // Record initialization routine
    DEVSUPFUN	get_ioint_info;  // Get io interrupt information
    DEVSUPFUN   write;           // Write routine
};//end LongOutDSET

/**************************************************************************************************/
/*                                Common Utility Routines                                         */
/*                                                                                                */


/**************************************************************************************************
|* parseLink () -- Parse An EPICS Database Link Structure
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    o Parse the record's EPICS Database input (INP) or output (OUT) field.
|*      The following sanity checks are performed on the link field:
|*       - Make sure the link type is INST_IO
|*       - Make sure the parameter field is not empty
|*       - Make sure the record function field is not empty
|*       - Make sure the event name field is not empty.
|*
|*    o Lookup the BasicSequenceEvent and BasicSequence objects that belong to this record.
|*      If these objects do not already exist, they will be created.
|*
|*    o Create a device information structure and initialize it with pointers to the
|*      BasicSequenceEvent object and the BasicSequenceEvent object associated with
|*      this record.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    pDevInfo = parseLink (dbLink, Function);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    dbLink   = (DBLINK &)     Reference to the record's INP or OUT link field.
|*
|*-------------------------------------------------------------------------------------------------
|* OUTPUT PARAMETERS:
|*    Function = (string &)     The record function string
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*    pDevInfo = (devInfoStruct *) Pointer to partially filled in device information structure
|*
|*-------------------------------------------------------------------------------------------------
|* EXCEPTIONS:
|*    A runtime_error is thrown if there was an error parsing the I/O link field
|*
\**************************************************************************************************/

static
devInfoStruct* parseLink (
    const DBLINK&        dbLink,             // Reference to the EPICS database I/O link field
    std::string&         Function)           // Reference to returned function name string
{
    //=====================
    // Local variables
    //
    epicsInt32           Card;               // Logical EVG card number (integer)
    std::string          CardString;         // Logical EVG card number (string)
    epicsInt32           Ram;                // Sequence RAM number (integer)
    std::string          RamString;          // Sequence RAM number (string)
    mrfIoLink*           ioLink = NULL;      // I/O link parsing object
    devInfoStruct*       pDevInfo = NULL;    // Pointer to device-specific information structure.
    EVG*                 pEvg = NULL;        // Pointer to event generator object.

    //=====================
    // Make sure the link type is correct
    //
    if (INST_IO != dbLink.type)
        throw std::runtime_error("I/O link type is not INST_IO");

    //=====================
    // Try to parse the link field
    //
    try {

        //=====================
        // Parse the link field
        //
        ioLink = new mrfIoLink (dbLink.value.instio.string, SeqRamParmNames, SeqRamNumParms);
        Card       = ioLink->getInteger ("C"   );
        CardString = ioLink->getString  ("C"   );
        Ram        = ioLink->getInteger ("Ram" );
        RamString  = ioLink->getString  ("Ram" );
        Function   = ioLink->getString  ("Fn"  );
        delete ioLink;

        //=====================
        // Get the event generator object for this sequence RAM
        //
        if (NULL == (pEvg = EgGetCard (Card)))
            throw std::runtime_error ("Event generator card " + CardString + " not configured");

        //=====================
        // Create and initialize the device information structure
        //
        pDevInfo = new devInfoStruct;
        pDevInfo->pEvg = pEvg;

        //=====================
        // Return the address of the device information structure
        //
        return pDevInfo;

    }//end try to parse the I/O link field

    //=====================
    // Catch any link parsing errors:
    //  - Delete the ioLink object
    //  - Delete the device information structure
    //  - Rethrow the error
    //
    catch (std::exception& e) {
        if (NULL != ioLink)   delete ioLink;
        if (NULL != pDevInfo) delete pDevInfo;
        throw std::runtime_error(e.what());
    }//end if there was an error parsing the link
    
}//end parseLink()

/**************************************************************************************************/
/*                      Device Support for Sequence RAM Binary Input Records                      */
/*                                                                                                */


/**************************************************************************************************/
/*  Binary Input Device Support Routines                                                          */
/**************************************************************************************************/

static epicsStatus biInitRecord (biRecord* pRec);
static epicsStatus biRead       (biRecord* pRec);


/**************************************************************************************************/
/*  Device Support Entry Table (DSET) For Binary Input Records                                    */
/**************************************************************************************************/

extern "C" {
static
BinaryDSET devBiSeqRam = {
    5,                                  // Number of entries in the table
    NULL,                               // -- No device report routine
    NULL,                               // -- No device support initialization routine
    (DEVSUPFUN)biInitRecord,            // Record initialization routine
    NULL,                               // -- No get I/O interrupt information routine
    (DEVSUPFUN)biRead                   // Read routine
};

epicsExportAddress (dset, devBiSeqRam);

};//end extern "C"

/**************************************************************************************************
|* biInitRecord () -- Initialize a Binary Input Record
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    This routine is called during IOC initialization to initialize a SequenceRAM
|*    binary input record.  It performs the following functions:
|*      - Parse the OUT field.
|*      - Register the record as this event's enable record.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLEMENTED FUNCTIONS:
|*    Enable  = Enables or disables this event.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    status = biInitRecord (pRec);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    pRec   = (biRecord *)     Address of the bi record structure
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*    status = (epicsStatus)    OK:     Successful initialization.
|*                              Other:  Failure code
|*
\**************************************************************************************************/

static
epicsStatus biInitRecord (biRecord *pRec)
{
    //=====================
    // Local variables
    //
    std::string          Function;           // Record function name (from OUT link)
    devInfoStruct*       pDevInfo = NULL;    // Pointer to device-specific information structure.
    epicsStatus          status;             // Anticipated error status

    //=====================
    // Try to initialize the record
    //
    try {

        //=====================
        // Parse the OUT link
        //
        status = S_dev_badOutType;
        pDevInfo = parseLink (pRec->out, Function);

        //=====================
        // Make sure the function code is correct
        //
        if ("Enable" != Function)
            throw std::runtime_error("Invalid record function (" + Function + ")");

        //=====================
        // Register the enable record
        //
        status = S_dev_Conflict;
        pDevInfo->pEvent->RegisterEnableRecord ((dbCommon *)pRec);

        //=====================
        // Attach the device information structure to the record.
        //
        pRec->dpvt = (void *)pDevInfo;

        //=====================
        // Set RVAL and return success
        //
        pRec->rval = (epicsUInt32)pRec->val;
        return OK;

    }//end try

    //=====================
    // If record initialization failed, report the error,
    // disable the record, and return the error status code
    //
    catch (std::exception& e) {
        recGblRecordError (status, pRec, (std::string("\nReason: ") + e.what()).c_str());
        mrfDisableRecord ((dbCommon *)pRec);
        if (NULL != pDevInfo) delete pDevInfo;
        return (status);
    }//end if record initialization failed

}//end biInitRecord()

/**************************************************************************************************
|* biRead () -- Read the Enable/Disable State to the SequenceRAM Object.
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    This routine is called from the biRecord's "process()" routine.
|*
|*    It converts the timestamp in the VAL field from engineering units to event clock ticks
|*    and reads it to the SequenceRAM object.
|*
|*    Note that the "raw" value (ticks) is represented by a 64-bit floating point number. This is
|*    because a sequence event timestamp can be as big as 2**44 ticks.  This means that we can't
|*    use the bi record's built in linear, slope, or breakpoint table conversion features.
|*    Consequently, the LINR field should be set to "NO CONVERSION".  This will disable the
|*    special linear conversion routine, so we recompute ESLO every time we are called (just in
|*    case EGUF was changed).
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLEMENTED FUNCTIONS:
|*    Enable  = Enables or disables this event.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    status = biRead (pRec);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    pRec   = (biRecord *)     Address of the bi record structure
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*    status = (epicsStatus)    Always returns OK
|*
\**************************************************************************************************/

static
epicsStatus biRead (biRecord* pRec) {

    //=====================
    // Extract the SequenceRAM object from the dpvt structure
    //
    devInfoStruct*       pDevInfo  = static_cast<devInfoStruct*>(pRec->dpvt);
    SequenceRAM*  pEvent    = pDevInfo->pEvent;

    //=====================
    // Convert the VAL field to a biolean and
    // read it to the SequenceRAM object.
    //
    pEvent->SetEventEnable (0 != pRec->val);

    return OK;

}//end biRead()


