/**************************************************************************************************
|* $(TIMING)/evgApp/src/devSequenceEvent.cpp -- EPICS Device Support for EVG Sequence Events
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
|*    This module contains EPICS device support for event generator Sequence Event objects.
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
/*  devSequenceEvent File Description                                                             */
/**************************************************************************************************/

//==================================================================================================
//! @addtogroup Sequencer
//! @{
//!
//==================================================================================================
//! @file       devSequenceEvent.cpp
//! @brief      EPICS Device Suppport for Sequence Events.
//!
//! @par Description:
//!   This file contains EPICS device support for event generator sequence events.
//!   A sequence event can be represented in the EPICS data base by two required records and
//!   up to three more optional records:
//!   - \b EVENT_CODE     (longout, required) Which event code to transmit.
//!   - \b EVENT_TIME     (ao, required) Specifies when in the sequence the event
//!                       should be transmitted.
//!   - \b EVENT_TIME     (ai, optional) Displays the actual timestamp assigned to the event.
//!                       This could differ from the requested timestamp because of event clock
//!                       quantization or because another event was assigned to that time.
//!   - \b EVENT_ENABLE   (bo, optional) Enables or disables event transmission.
//!   - \b EVENT_PRIORITY (longout, optional) When two sequence events end up with the same
//!                       timestamp, the relative priorities will determine which one gets
//!                       "jostled".
//!   @par
//!   Every event in a sequence must have a unique name associated with it -- even if the
//!   event code is duplicated.  The unique name is assigned when the SequenceEvent object is
//!   created.
//!
//! @par Device Type
//!   A sequence event record will have the "Device Type" field (DTYP) set to:
//!   @verbatim
//!      "EVG Sequence Event"
//!   @endverbatim
//!
//! @par Link Format
//!   For sequence event records, the INP or OUT link has the following format:
//!   @verbatim
//!      #C0 Ss @FUNCTION Name
//!   @endverbatim
//!   Where:
//!   - \b C0         = Not used
//!   - \b Ss         = Specifies the ID number of the sequence that this event belongs to
//!   - \b \@FUNCTION = The record's function name (see list above)
//!   - \b Name       = The sequence event name.
//!   @par
//!   Note that the sequence event name may contain blanks.
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
#include  <initHooks.h>         // EPICS IOC Initialization hooks support library
#include  <iocsh.h>             // EPICS IOC shell support library
#include  <link.h>              // EPICS Database link definitions
#include  <recGbl.h>            // EPICS Global record support routines
#include  <registryFunction.h>  // EPICS Registry support library

#include  <aiRecord.h>          // EPICS Analog input record definition
#include  <aoRecord.h>          // EPICS Analog output record definition
#include  <boRecord.h>          // EPICS Binary output record definition
#include  <longoutRecord.h>     // EPICS Long output record definition

#include  <menuConvert.h>       // EPICS conversion field menu items

#include  <mrfCommon.h>         // MRF Common definitions
#include  <mrfIoLink.h>         // MRF I/O link field parser

#include  <Sequence.h>          // MRF Sequence class
#include  <SequenceEvent.h>     // MRF Sequence event class
#include  <devSequence.h>       // MRF Sequence device support declarations
#include  <drvEvg.h>            // MRF Event Generator driver infrastructure routines

#include  <epicsExport.h>       // EPICS Symbol exporting macro definitions

/**************************************************************************************************/
/*  Structure Definitions                                                                         */
/**************************************************************************************************/

//=====================
// Common I/O link parameter definitions used by all SequenceEvent records
//
static const
mrfParmNameList SeqEventParmNames = {
    "C",        // Logical card number
    "Name",     // Event name
    "Fn",       // Record function code
    "Seq"       // Sequence number that this event belongs to
};

static const
epicsInt32  SeqEventNumParms mrfParmNameListSize(SeqEventParmNames);


//=====================
// Common device information structure used by all SequenceEvent records
//
struct devInfoStruct {
    Sequence       *pSequence;  // Pointer to the Sequence object
    SequenceEvent  *pEvent;     // Pointer to the SequenceEvent object
    epicsInt32      Function;   // Function code
};//end devInfoStruct


//=====================
// Device Support Entry Table (DSET) for analog input and analog output records
//
struct AnalogDSET {
    long	number;	         // Number of support routines
    DEVSUPFUN	report;		 // Report routine
    DEVSUPFUN	init;	         // Device suppport initialization routine
    DEVSUPFUN	init_record;     // Record initialization routine
    DEVSUPFUN	get_ioint_info;  // Get io interrupt information
    DEVSUPFUN   perform_io;      // Read or Write routine
    DEVSUPFUN   special_linconv; // Special linear-conversion routine
};//end AnalogDSET

/**************************************************************************************************/
/*                                Common Utility Routines                                         */
/*                                                                                                */


/**************************************************************************************************
|* parseLink () -- Parse An EPICS Database Link Structure
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    Parse the record's EPICS Database input (INP) or output (OUT) field and return the
|*    sequence number this event is assigned to, the record function field, and the event name.
|*
|*    The following sanity checks are performed on the link field:
|*      - Make sure the link type is INST_IO
|*      - Make sure the parameter field is not empty
|*      - Make sure the record function field is not empty
|*      - Make sure the event name field is not empty.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    parseLink (dbLink, Card, Function, Name, SeqNum);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    dbLink   = (DBLINK &)     Reference to the record's INP or OUT link field.
|*
|*-------------------------------------------------------------------------------------------------
|* OUTPUT PARAMETERS:
|*    Card     = (epicsInt32 &) The EVG logical card number
|*    Function = (string &)     The record function string
|*    Name     = (string &)     The sequence event name
|*    SeqNum   = (epicsInt32 &) The ID number of the sequence this event belongs to
|*
|*-------------------------------------------------------------------------------------------------
|* EXCEPTIONS:
|*    A runtime_error is thrown if there was an error parsing the I/O link field
|*
\**************************************************************************************************/

static
void parseLink (
    const DBLINK&  dbLink,      // Reference to the EPICS database I/O link field
    epicsInt32&    Card,        // Reference to returned logical card number
    std::string&   Function,    // Reference to returned function name string
    std::string&   Name,        // Reference to the returned sequence event name string
    epicsInt32&    SeqNum)      // Reference to the returned sequence ID number
{
    //=====================
    // Local variables
    //
    mrfIoLink*   ioLink = NULL; // I/O link parsing object

    //=====================
    // Make sure the link type is correct
    //
    if (INST_IO != dbLink.type)
        throw std::runtime_error("I/O link type is not INST_IO");

    //=====================
    // Parse the link field
    //
    try {
        ioLink   = new mrfIoLink (dbLink.value.instio.string, SeqEventParmNames, SeqEventNumParms);
        Name     = ioLink->getString  ("Name");
        Function = ioLink->getString  ("Fn"  );
        SeqNum   = ioLink->getInteger ("Seq" );
        Card     = ioLink->getInteger ("C"   );
        delete ioLink;
    }//end try to parse the I/O link field

    //=====================
    // Catch any link parsing errors:
    //  - Delete the ioLink object
    //  - Rethrow the error
    //
    catch (std::exception& e) {
        delete ioLink;
        throw std::runtime_error(e.what());
    }//end if there was an error parsing the link
    
}//end parseLink()

/**************************************************************************************************/
/*                    Device Support for Sequence Event Analog Output Records                     */
/*                                                                                                */


/**************************************************************************************************/
/*  Analog Output Devise Support Routines                                                         */
/**************************************************************************************************/

static epicsStatus aoInitRecord (aoRecord* pRec);
static epicsStatus aoWrite      (aoRecord* pRec);


/**************************************************************************************************/
/*  Device Support Entry Table (DSET) For Analog Output Records                                   */
/**************************************************************************************************/

extern "C" {
static
AnalogDSET devAoSeqEvent = {
    6,                                  // Number of entries in the table
    NULL,                               // -- No device report routine
    NULL,                               // -- No device support initialization routine
    (DEVSUPFUN)aoInitRecord,            // Record initialization routine
    NULL,                               // -- No get I/O interrupt information routine
    (DEVSUPFUN)aoWrite,                 // Write routine
    NULL                                // -- No special linear conversion routine
};

epicsExportAddress (dset, devAoSeqEvent);

};//end extern "C"

/**************************************************************************************************
|* aoInitRecord () -- Initialize an Analog Output Record
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    This routine is called during IOC initialization to initialize a Sequence Event
|*    analog output record.  It performs the following functions:
|*      - Extract the card number, sequence number, record function, and event name from
|*        the OUT field.
|*      - Declare the sequence number and the event name.
|*      - Register the record as this event's timestamp record.
|*      - Create and initialize the device information structure.
|*      - Initialize the LINR and ESLO fields.
|*      - Call the aoWrite routine to convert the initial value to ticks and
|*        write it to the SequenceEvent object.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLEMENTED FUNCTIONS:
|*    EVENT_TIME  = Sets the requested time for this event to occur.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    status = aoInitRecord (pRec);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    pRec   = (aoRecord *)     Address of the ao record structure
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*    status = (epicsStatus)    NO_CONVERT:  Successful initialization, do not perform
|*                                           value conversion
|*                              Other        Failure code
|*
\**************************************************************************************************/

static
epicsStatus aoInitRecord (aoRecord *pRec)
{
    //=====================
    // Local variables
    //
    epicsInt32      Card;               // Logical EVG card number (from OUT link)
    std::string     Function;           // Record function name (from OUT link)
    std::string     Name;               // Sequence event name (from OUT link)
    devInfoStruct*  pDevInfo = NULL;    // Pointer to device-specific information structure.
    Sequence*       pSequence;          // Pointer to the Sequence object for this event
    SequenceEvent*  pEvent;             // Pointer to our SequenceEvent object
    epicsInt32      SeqNum;             // Sequence ID number for this event (from OUT link)
    epicsStatus     status;             // Anticipated error status

    //=====================
    // Try to initialize the record
    //
    try {

        //=====================
        // Parse the OUT link and extract the record function, event name, and sequence number.
        //
        status = S_dev_badOutType;
        parseLink (pRec->out, Card, Function, Name, SeqNum);

        //=====================
        // Make sure the function code is correct
        //
        if ("Time" != Function)
            throw std::runtime_error("Invalid record function (" + Function + ")");

        //=====================
        // Declare the sequence number and the event name
        //
        status = -1;
        pSequence = EgDeclareSequence (Card, SeqNum);
        pEvent = pSequence->DeclareEvent (Name);

        //=====================
        // Register the timestamp record
        //
        status = S_dev_Conflict;
        pEvent->RegisterTimeRecord ((dbCommon *)pRec);

        //=====================
        // Create the device information structure
        //
        status = S_db_noMemory;
        if (NULL == (pDevInfo = (devInfoStruct *)malloc(sizeof(devInfoStruct))))
            throw std::runtime_error("Unable to create device information structure");

        //=====================
        // Fill in the device information structure and attach it to the record.
        //
        pDevInfo->pSequence = pSequence;
        pDevInfo->pEvent = pEvent;
        pRec->dpvt = (void *)pDevInfo;

        //=====================
        // Disable automatic value conversion by record support.
        // We have to do our own conversion because the raw timestamp could
        // exceed 32 bits.
        //
        pRec->linr = menuConvertNO_CONVERSION;

        //=====================
        // Set the initial value 
        //
        aoWrite (pRec);
        return NO_CONVERT;

    }//end try

    //=====================
    // If record initialization failed, report the error,
    // disable the record, and return the error status code
    //
    catch (std::exception& e) {
        recGblRecordError (status, pRec, (std::string("\nReason: ") + e.what()).c_str());
        mrfDisableRecord ((dbCommon *)pRec);
        return (status);
    }//end if record initialization failed

}//end aoInitRecord()

/**************************************************************************************************
|* aoWrite () -- Write the Timestamp to the SequenceEvent Object.
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    This routine is called from the aoRecord's "process()" routine.
|*
|*    It converts the timestamp in the VAL field from engineering units to event clock ticks
|*    and writes it to the SequenceEvent object.
|*
|*    Note that the "raw" value (ticks) is represented by a 64-bit floating point number. This is
|*    because a sequence event timestamp can be as big as 2**44 ticks.  This means that we can't
|*    use the ao record's built in linear, slope, or breakpoint table conversion features.
|*    Consequently, the LINR field should be set to "NO CONVERSION".  This will disable the
|*    special linear conversion routine, so we recompute ESLO every time we are called (just in
|*    case EGUF was changed).
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLEMENTED FUNCTIONS:
|*    EVENT_TIME  = Sets the requested time for this event to occur.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    status = aoWrite (pRec);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    pRec   = (aoRecord *)     Address of the ao record structure
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*    status = (epicsStatus)    Always returns OK
|*
\**************************************************************************************************/

static
epicsStatus aoWrite (aoRecord* pRec) {

    //=====================
    // Extract the Sequence and SequenceEvent objects from the dpvt structure
    //
    devInfoStruct*  pDevInfo  = static_cast<devInfoStruct*>(pRec->dpvt);
    Sequence*       pSequence = pDevInfo->pSequence;
    SequenceEvent*  pEvent    = pDevInfo->pEvent;

    //=====================
    // Re-compute the slope (in case EGUF has changed)
    // If EGUF is 0, it means we want event clock ticks.
    //
    if (0.0 == pRec->eguf)
        pRec->eslo = 1.0;

    else
        pRec->eslo = pSequence->GetSecsPerTick() / pRec->eguf;

    //=====================
    // Convert the VAL field to "Event Clock Ticks" and write
    // it to the SequenceEvent object.
    //
    pEvent->SetEventTime (pRec->val / pRec->eslo);

    return OK;

}//end aoWrite()

//!
//! @}
//end group Sequencer
