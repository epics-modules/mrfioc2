/**************************************************************************************************
|* mrfVme64x.c -- Utilities to Support VME-64X CR/CSR Geographical Addressing
|*
|*-------------------------------------------------------------------------------------------------
|* Authors:  Jukka Pietarinen (Micro-Research Finland, Oy)
|*           Till Straumann (SLAC)
|*           Eric Bjorklund (LANSCE)
|* Date:     15 May 2006
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 15 May 2006  E.Bjorklund     Original, Adapted from Jukka's vme64x_cr.c program
|* 24 Aug 2006  S.Allison       Added iocsh registration of utility routines
|* 12 Oct 2007  R.Hartmann      Updated to include the series 230 modules
|* 26 Aug 2009  E.Bjorklund     Removed the EVG-230 from the list of modules that support
|*                              function 1 addressing in CSR space.
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This module contains utility routines for reading and manipulating the VME CR/CSR space
|* used by the MRF Series 200 modules to configure their VME address spaces.
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
|* Copyright (c) 2008 The University of Chicago,
|* as Operator of Argonne National Laboratory.
|*
|* Copyright (c) 2008 The Regents of the University of California,
|* as Operator of Los Alamos National Laboratory.
|*
|* Copyright (c) 2008 The Board of Trustees of the Leland Stanford Junior
|* University, as Operator of the Stanford Linear Accelerator Center.
|*
|**************************************************************************************************
|*
|* This software is distributed under the EPICS Open License Agreement which
|* can be found in the file, LICENSE, included with this distribution.
|*
\*************************************************************************************************/

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <stddef.h>             /* Standard C definitions                                         */
#include <string.h>             /* Standard C string library                                      */

#include <iocsh.h>              /* EPICS IOC shell support library                                */

#include <epicsStdlib.h>        /* EPICS Standard C library support routines                      */
#include <epicsStdio.h>         /* EPICS Standard C I/O support routines                          */
#include <epicsTypes.h>         /* EPICS Architecture-independent type definitions                */
#include <epicsExport.h>        /* EPICS Symbol exporting macro definitions                       */

#include <mrfVme64x.h>          /* VME-64X CR/CSR routines and definitions (with MRF extensions)  */
#include <mrfCommonIO.h>        /* MRF Common I/O macro definitions                               */


/**************************************************************************************************/
/*  CR/CSR Configuration Constants                                                                */
/**************************************************************************************************/

#define  MRF_USER_CSR_DEFAULT     0x7fb03 /* Default offset to MRF User-CSR space                 */
#define  VME_64X_NUM_FUNCTIONS          8 /* Number of defined functions in VME-64X CR/CSR space  */


/**************************************************************************************************/
/*  Macro Definitions                                                                             */
/**************************************************************************************************/

/*---------------------
 * Macro to compute the CR/CSR base address for a slot
 */
#define CSR_BASE(slot) (slot << 19)

/*---------------------
 * Macro to compute the longword index for a given CR/CSR byte address
 */
#define LINDEX(addr) ((addr) >> 2)


/**************************************************************************************************/
/*  Prototypes for Local Utility Routines                                                         */
/**************************************************************************************************/

static  epicsStatus        convCRtoLong     (epicsUInt32*, epicsInt32, epicsUInt32*);
static  epicsStatus        convOffsetToLong (epicsUInt32*, epicsInt32, epicsUInt32*);

static  void               getSNString      (epicsUInt32*, char*);
static  epicsUInt32        getUserCSRAdr    (epicsInt32);

static  epicsStatus        probeCRSpace     (epicsInt32, epicsUInt32*);
static  epicsStatus        probeUCSRSpace   (epicsInt32, epicsUInt32*);


/**************************************************************************************************/
/*  Local Constants                                                                               */
/**************************************************************************************************/

/*---------------------
 * Table for translating binary into hexadecimal characters
 */
static const char hexChar [16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

/*---------------------
 * Table of VME cards that support function 1
 */
static const epicsUInt32 funcOneCards [] = {
    MRF_VME_EVR230_BID,          /* Series 230 Event Receiver                                     */
    MRF_VME_EVR230RF_BID         /* Series 230 Event Receiver with CML outputs                    */
};

#define MRF_NUM_FUNC_ONE_CARDS (sizeof(funcOneCards) / sizeof(epicsUInt32))

/**************************************************************************************************/
/*  CR/CSR Configuration ROM (CR) Register Definitions                                            */
/**************************************************************************************************/

#define  CR_ROM_CHECKSUM           0x0003 /* 8-bit checksum of Configuration ROM space            */
#define  CR_ROM_LENGTH             0x0007 /* Number of bytes in Configuration ROM to checksum     */
#define  CR_DATA_ACCESS_WIDTH      0x0013 /* Configuration ROM area (CR) data access method       */
#define  CSR_DATA_ACCESS_WIDTH     0x0017 /* Control/Status Reg area (CSR) data access method     */
#define  CR_SPACE_ID               0x001B /* CR/CSR space ID (VME64, VME64X, etc).                */

#define  CR_ASCII_C                0x001F /* ASCII "C" (identifies this as CR space)              */
#define  CR_ASCII_R                0x0023 /* ASCII "R" (identifies this as CR space)              */

#define  CR_IEEE_OUI               0x0027 /* IEEE Organizationally Unique Identifier (OUI)        */
#define  CR_BOARD_ID               0x0033 /* Manufacturer's board ID                              */
#define  CR_REVISION_ID            0x0043 /* Manufacturer's board revision ID                     */
#define  CR_ASCII_STRING           0x0053 /* Offset to ASCII string (manufacturer-specific)       */
#define  CR_PROGRAM_ID             0x007F /* Program ID code for CR space                         */

#define  CR_BEG_UCR                0x0083 /* Offset to start of manufacturer-defined CR space     */
#define  CR_END_UCR                0x008F /* Offset to end of manufacturer-defined CR space       */

#define  CR_BEG_CRAM               0x009B /* Offset to start of Configuration RAM (CRAM) space    */
#define  CR_END_CRAM               0x00A7 /* Offset to end of Configuration RAM (CRAM) space      */

#define  CR_BEG_UCSR               0x00B3 /* Offset to start of manufacturer-defined CSR space    */
#define  CR_END_UCSR               0x00BF /* Offset to end of manufacturer-defined CSR space      */

#define  CR_BEG_SN                 0x00CB /* Offset to beginning of board serial number           */
#define  CR_END_SN                 0x00D7 /* Offset to end of board serial number                 */

#define  CR_SLAVE_CHAR             0x00E3 /* Board's slave-mode characteristics                   */
#define  CR_UD_SLAVE_CHAR          0x00E7 /* Manufacturer-defined slave-mode characteristics      */

#define  CR_MASTER_CHAR            0x00EB /* Board's master-mode characteristics                  */
#define  CR_UD_MASTER_CHAR         0x00EF /* Manufacturer-defined master-mode characteristics     */

#define  CR_IRQ_HANDLER_CAP        0x00F3 /* Interrupt levels board can respond to (handle)       */
#define  CR_IRQ_CAP                0x00F7 /* Interrupt levels board can assert                    */

#define  CR_CRAM_WIDTH             0x00FF /* Configuration RAM (CRAM) data access method)         */

#define  CR_FN_DAWPR               0x0103 /* Start of Data Access Width Parameter (DAWPR) regs    */
#define  CR_F0_DAWPR               0x0103 /* Data Access Width Parameters for function 0          */
#define  CR_F1_DAWPR               0x0107 /* Data Access Width Parameters for function 1          */
#define  CR_F2_DAWPR               0x010B /* Data Access Width Parameters for function 2          */
#define  CR_F3_DAWPR               0x010F /* Data Access Width Parameters for function 3          */
#define  CR_F4_DAWPR               0x0113 /* Data Access Width Parameters for function 4          */
#define  CR_F5_DAWPR               0x0117 /* Data Access Width Parameters for function 5          */
#define  CR_F6_DAWPR               0x011B /* Data Access Width Parameters for function 6          */
#define  CR_F7_DAWPR               0x011F /* Data Access Width Parameters for function 7          */

#define  CR_FN_AMCAP               0x0123 /* Start of Address Mode Capability (AMCAP) registers   */
#define  CR_F0_AMCAP               0x0123 /* Address Mode Capabilities for function 0             */
#define  CR_F1_AMCAP               0x0143 /* Address Mode Capabilities for function 1             */
#define  CR_F2_AMCAP               0x0163 /* Address Mode Capabilities for function 2             */
#define  CR_F3_AMCAP               0x0183 /* Address Mode Capabilities for function 3             */
#define  CR_F4_AMCAP               0x01A3 /* Address Mode Capabilities for function 4             */
#define  CR_F5_AMCAP               0x01C3 /* Address Mode Capabilities for function 5             */
#define  CR_F6_AMCAP               0x01E3 /* Address Mode Capabilities for function 6             */
#define  CR_F7_AMCAP               0x0203 /* Address Mode Capabilities for function 7             */

#define  CR_FN_XAMCAP              0x0223 /* Start of Extended Address Mode Cap (XAMCAP) registers*/
#define  CR_F0_XAMCAP              0x0223 /* Extended Address Mode Capabilities for function 0    */
#define  CR_F1_XAMCAP              0x02A3 /* Extended Address Mode Capabilities for function 1    */
#define  CR_F2_XAMCAP              0x0323 /* Extended Address Mode Capabilities for function 2    */
#define  CR_F3_XAMCAP              0x03A3 /* Extended Address Mode Capabilities for function 3    */
#define  CR_F4_XAMCAP              0x0423 /* Extended Address Mode Capabilities for function 4    */
#define  CR_F5_XAMCAP              0x04A3 /* Extended Address Mode Capabilities for function 5    */
#define  CR_F6_XAMCAP              0x0523 /* Extended Address Mode Capabilities for function 6    */
#define  CR_F7_XAMCAP              0x05A3 /* Extended Address Mode Capabilities for function 7    */

#define  CR_FN_ADEM                0x0623 /* Start of Address Decoder Mask (ADEM) registers       */
#define  CR_F0_ADEM                0x0623 /* Address Decoder Mask for function 0                  */
#define  CR_F1_ADEM                0x0633 /* Address Decoder Mask for function 1                  */
#define  CR_F2_ADEM                0x0643 /* Address Decoder Mask for function 2                  */
#define  CR_F3_ADEM                0x0653 /* Address Decoder Mask for function 3                  */
#define  CR_F4_ADEM                0x0663 /* Address Decoder Mask for function 4                  */
#define  CR_F5_ADEM                0x0673 /* Address Decoder Mask for function 5                  */
#define  CR_F6_ADEM                0x0683 /* Address Decoder Mask for function 6                  */
#define  CR_F7_ADEM                0x0693 /* Address Decoder Mask for function 7                  */

#define  CR_MASTER_DAWPR           0x06AF /* Master Data Access Width Parameter                   */
#define  CR_MASTER_AMCAP           0x06B3 /* Master Address Mode Capabilities                     */
#define  CR_MASTER_XAMCAP          0x06D3 /* Master Extended Address Mode Capabilities            */

/*---------------------
 * Number of bytes and total size of manufacturer's Organizationally Unique Identifier (OUI)
 */
#define  CR_IEEE_OUI_BYTES                     3   /* Number of bytes in manufacturer's OUI       */
#define  CR_IEEE_OUI_SIZE (CR_IEEE_OUI_BYTES * 4)  /* Size manufacturer's OUI (in bytes)          */

/*---------------------
 * Number of bytes and total size of manufacturer's board ID
 */
#define  CR_BOARD_ID_BYTES                     4   /* Number of bytes in manufacturer's OUI       */
#define  CR_BOARD_ID_SIZE (CR_BOARD_ID_BYTES * 4)  /* Size manufacturer's OUI (in bytes)          */

/*---------------------
 * Number of bytes and total size of the offset to the start of User CSR space
 */
#define  CR_BEG_UCSR_BYTES                     3   /* Number of bytes in User CSR starting offset */
#define  CR_BEG_UCSR_SIZE (CR_BEG_UCSR_BYTES * 4)  /* Size of User CSR starting offset (in bytes) */

/*---------------------
 * Number of bytes and total size of a Data Access Width Parameter (DAWPR) Register
 */
#define  CR_DAWPR_BYTES                        1   /* Number of bytes in a DAWPR register         */
#define  CR_DAWPR_SIZE       (CR_DAWPR_BYTES * 4)  /* Size of DAWPR register (in bytes)           */

/*---------------------
 * Number of bytes and total size of an Address Mode Capability (AMCAP) Register
 */
#define  CR_AMCAP_BYTES                        8   /* Number of bytes in an AMCAP register        */
#define  CR_AMCAP_SIZE       (CR_AMCAP_BYTES * 4)  /* Size of AMCAP register (in bytes)           */

/*---------------------
 * Number of bytes and total size of an Extended Address Mode Capability (XAMCAP) Register
 */
#define  CR_XAMCAP_BYTES                      32   /* Number of bytes in an XAMCAP register       */
#define  CR_XAMCAP_SIZE     (CR_XAMCAP_BYTES * 4)  /* Size of XAMCAP register (in bytes)          */

/*---------------------
 * Number of bytes and total size of an Address Decoder Mask (ADEM) Register
 */
#define  CR_ADEM_BYTES                         4   /* Number of bytes in an ADEM register         */
#define  CR_ADEM_SIZE         (CR_ADEM_BYTES * 4)  /* Size of ADEM register (in bytes)            */

/*---------------------
 * Size (in total bytes) of CR space
 */
#define  CR_SIZE                          0x0750   /* Size of CR space (in total bytes)           */
#define  CR_BYTES                 LINDEX(CR_SIZE)  /* Number of bytes in CR space                 */

/**************************************************************************************************/
/*  CR/CSR Control and Status Register (CSR) Offsets                                              */
/**************************************************************************************************/

#define  CSR_BAR                0x7ffff /* Base Address Register (MSB of our CR/CSR address)      */
#define  CSR_BIT_SET            0x7fffb /* Bit Set Register (writing a 1 sets the control bit)    */
#define  CSR_BIT_CLEAR          0x7fff7 /* Bit Clear Register (writing a 1 clears the control bit)*/
#define  CSR_CRAM_OWNER         0x7fff3 /* Configuration RAM Owner Register (0 = not owned)       */
#define  CSR_UD_BIT_SET         0x7ffef /* User-Defined Bit Set Register (for user-defined fns)   */
#define  CSR_UD_BIT_CLEAR       0x7ffeb /* User-Defined Bit Clear Register (for user-defined fns) */
#define  CSR_FN7_ADER           0x7ffd3 /* Function 7 Address Decoder Compare Register (1st byte) */
#define  CSR_FN6_ADER           0x7ffc3 /* Function 6 Address Decoder Compare Register (1st byte) */
#define  CSR_FN5_ADER           0x7ffb3 /* Function 5 Address Decoder Compare Register (1st byte) */
#define  CSR_FN4_ADER           0x7ffa3 /* Function 4 Address Decoder Compare Register (1st byte) */
#define  CSR_FN3_ADER           0x7ff93 /* Function 3 Address Decoder Compare Register (1st byte) */
#define  CSR_FN2_ADER           0x7ff83 /* Function 2 Address Decoder Compare Register (1st byte) */
#define  CSR_FN1_ADER           0x7ff73 /* Function 1 Address Decoder Compare Register (1st byte) */
#define  CSR_FN0_ADER           0x7ff63 /* Function 0 Address Decoder Compare Register (1st byte) */

/*---------------------
 * Number of bytes and total size of Address Decoder Compare (ADER) Register
 */
#define  CSR_ADER_BYTES                     4   /* Number of bytes in an ADER register            */
#define  CSR_ADER_SIZE    (CSR_ADER_BYTES * 4)  /* Size of DAWPR register (in bytes)              */

/*---------------------
 * Bit offset definitions for the Bit Set Status Register
 */
#define  CSR_BITSET_RESET_MODE           0x80   /* Module is in reset mode                        */
#define  CSR_BITSET_SYSFAIL_ENA          0x40   /* SYSFAIL driver is enabled                      */
#define  CSR_BITSET_MODULE_FAIL          0x20   /* Module has failed                              */
#define  CSR_BITSET_MODULE_ENA           0x10   /* Module is enabled                              */
#define  CSR_BITSET_BERR                 0x08   /* Module has asserted a Bus Error                */
#define  CSR_BITSET_CRAM_OWNED           0x04   /* CRAM is owned                                  */


/**************************************************************************************************/
/*  CR/CSR User-CSR Space Offsets (MRF Specific)                                                  */
/**************************************************************************************************/

#define  UCSR_IRQ_LEVEL                0x0003   /* Interrupt request level                        */
#define  UCSR_SERIAL_NUMBER            0x0013   /* Board serial number (MAC Address)              */

#define  UCSR_SN_SIZE       (MRF_SN_BYTES * 4)  /* Size of serial number space (in total bytes)   */

#define  UCSR_SIZE                       0x28   /* Size of User CSR space (in total bytes)        */
#define  UCSR_BYTES          LINDEX(UCSR_SIZE)  /* Number of bytes in User CSR space              */

/**************************************************************************************************/
/*                      Global Functions Called By Driver and Device Support                      */
/*                                                                                                */


/**************************************************************************************************
|* mrfFindNextEVG () -- Locate the Next VME Slot Containing an Event Generator Card
|*-------------------------------------------------------------------------------------------------
|*
|* Locate the next VME slot, starting from the slot after the one specified in the input
|* parameter, which contains an Event Generator card of any series.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|* This routine assumes that the input parameter contains the slot number of the last
|* Event Generator card found in the crate.  Starting with the slot after the one specified
|* in the input parameter, it searches VME CR/CSR address space looking for an IEEE OUI code
|* that matches the MRF OUI, and a Board ID code that matches an Event Generator Board ID.
|* If an Event Generator card is found, the routine will return its slot number.  If no further
|* Event Generator cards are found after the starting slot, the routine will return 0.
|*
|* To locate the first Event Generator in the crate, the routine can be called with an input
|* parameter of either 0 or 1, since 0 is not a valid slot number and slot 1 typically contains
|* the IOC processor card.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      slot = mrfFindNextEVG (lastSlot);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      lastSlot  = (epicsInt32)  Slot number of the last VME slot identified as containing
|*                                an Event Generator card.  0, if we are just starting the search.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      slot      = (epicsInt32)  VME slot number of the next Event Generator card to be found.
|*                                0 if no more Event Generator cards were located in this crate.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine can be called from the vxWorks and RTEMS shells, but not from the IOC shell.
|* o The following code can be used to locate and initialize all Event Generator cards
|*   in the crate.
|*
|*        slot = 1;
|*        while (0 != (slot = mrfFindNextEVG(slot))) {
|*            Initialize Event Generator card at "slot";
|*        }
|*
\**************************************************************************************************/


epicsInt32 mrfFindNextEVG (epicsInt32 lastSlot)
{
    epicsInt32   slot;          /* VME slot number of next Event Generator Card                   */
    epicsStatus  status;        /* Local status variable                                          */

   /*---------------------
    * Make sure the input parameter is in the appropriate range
    */
    if ((lastSlot > 20) || (lastSlot < 0))
        return 0;

   /*---------------------
    * Locate the next Event Generator Card
    */
    status = vmeCRFindBoardMasked (
               lastSlot+1,              /* Start looking in the next slot to the left             */
               MRF_VME_IEEE_OUI,        /* MRF Vendor ID                                          */
               MRF_VME_EVG_BID,         /* Generic Event Generator board ID                       */
               0xffff0000,              /* Mask out the series number                             */
               &slot);                  /* Return slot number of next card found                  */

   /*---------------------
    * Return slot number of next Event Generator card found. 
    * Return 0 if no more Event Generator cards were found.
    */
    if (OK == status)
         return slot;
    else return 0;

}/*end mrfFindNextEVG()*/

/**************************************************************************************************
|* mrfFindNextEVG200 () -- Locate the Next VME Slot Containing a Series 200 Event Generator Card
|*-------------------------------------------------------------------------------------------------
|*
|* Locate the next VME slot, starting from the slot after the one specified in the input
|* parameter, which contains a series 200 Event Generator card.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|* This routine assumes that the input parameter contains the slot number of the last
|* series 200 Event Generator card found in the crate.  Starting with the slot after the
|* one specified in the input parameter, it searches VME CR/CSR address space looking for
|* an IEEE OUI code that matches the MRF OUI, and a Board ID code that matches the series 200
|* Event Generator Board ID.  If a series 200 Event Generator card is found, the routine will
|* return its slot number.  If no further series 200 Event Generator cards are found after
|* the starting slot, the routine will return 0.
|*
|* To locate the first series 200 Event Generator in the crate, the routine can be called with an
|* input parameter of either 0 or 1, since 0 is not a valid slot number and slot 1 typically
|* contains the IOC processor card.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      slot = mrfFindNextEVG200 (lastSlot);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      lastSlot  = (epicsInt32)  Slot number of the last VME slot identified as containing
|*                                an Event Generator card.  0, if we are just starting the search.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      slot      = (epicsInt32)  VME slot number of the next Event Generator card to be found.
|*                                0 if no more Event Generator cards were located in this crate.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine can be called from the vxWorks and RTEMS shells, but not from the IOC shell.
|* o The following code can be used to locate and initialize all series 200 Event Generator cards
|*   in the crate.
|*
|*        slot = 1;
|*        while (0 != (slot = mrfFindNextEVG200(slot))) {
|*            Initialize Event Generator card at "slot";
|*        }
|*
\**************************************************************************************************/


epicsInt32 mrfFindNextEVG200 (epicsInt32 lastSlot)
{
    epicsInt32   slot;          /* VME slot number of next Event Generator Card                   */

   /*---------------------
    * Make sure the input parameter is in the appropriate range
    */
    if ((lastSlot > 20) || (lastSlot < 0))
        return 0;

   /*---------------------
    * Locate the next Event Generator Card
    */
    if (OK == vmeCRFindBoard (lastSlot+1, MRF_VME_IEEE_OUI, MRF_VME_EVG200_BID, &slot))
        return slot;

   /*---------------------
    * Return 0 if no more Event Generator cards were found.
    */
    return 0;

}/*end mrfFindNextEVG200()*/

/**************************************************************************************************
|* mrfFindNextEVG230 () -- Locate the Next VME Slot Containing a Series 230 Event Generator Card
|*-------------------------------------------------------------------------------------------------
|*
|* Locate the next VME slot, starting from the slot after the one specified in the input
|* parameter, which contains a series 230 Event Generator card.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|* This routine assumes that the input parameter contains the slot number of the last
|* series 230 Event Generator card found in the crate.  Starting with the slot after the
|* one specified in the input parameter, it searches VME CR/CSR address space looking for
|* an IEEE OUI code that matches the MRF OUI, and a Board ID code that matches the series 230
|* Event Generator Board ID.  If a series 230 Event Generator card is found, the routine will
|* return its slot number.  If no further series 230 Event Generator cards are found after
|* the starting slot, the routine will return 0.
|*
|* To locate the first series 230 Event Generator in the crate, the routine can be called with an
|* input parameter of either 0 or 1, since 0 is not a valid slot number and slot 1 typically
|* contains the IOC processor card.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      slot = mrfFindNextEVG230 (lastSlot);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      lastSlot  = (epicsInt32)  Slot number of the last VME slot identified as containing
|*                                an Event Generator card.  0, if we are just starting the search.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      slot      = (epicsInt32)  VME slot number of the next Event Generator card to be found.
|*                                0 if no more Event Generator cards were located in this crate.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine can be called from the vxWorks and RTEMS shells, but not from the IOC shell.
|* o The following code can be used to locate and initialize all series 230 Event Generator cards
|*   in the crate.
|*
|*        slot = 1;
|*        while (0 != (slot = mrfFindNextEVG230(slot))) {
|*            Initialize Event Generator card at "slot";
|*        }
|*
\**************************************************************************************************/


epicsInt32 mrfFindNextEVG230 (epicsInt32 lastSlot)
{
    epicsInt32   slot;          /* VME slot number of next Event Generator Card                   */

   /*---------------------
    * Make sure the input parameter is in the appropriate range
    */
    if ((lastSlot > 20) || (lastSlot < 0))
        return 0;

   /*---------------------
    * Locate the next Event Generator Card
    */
    if (OK == vmeCRFindBoard (lastSlot+1, MRF_VME_IEEE_OUI, MRF_VME_EVG230_BID, &slot))
        return slot;

   /*---------------------
    * Return 0 if no more Event Generator cards were found.
    */
    return 0;

}/*end mrfFindNextEVG230()*/

/**************************************************************************************************
|* mrfFindNextEVR () -- Locate the Next VME Slot Containing an Event Receiver Card
|*-------------------------------------------------------------------------------------------------
|*
|* Locate the next VME slot, starting from the slot after the one specified in the input
|* parameter, which contains an Event Receiver card of either series.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|* This routine assumes that the input parameter contains the slot number of the last
|* series Event Receiver card found in the crate.  Starting with the slot after the
|* one specified in the input parameter, it searches VME CR/CSR address space looking for
|* an IEEE OUI code that matches the MRF OUI, and a Board ID code that matches any Event Receiver
|* Board ID.  If an Event Receiver card is found, the routine will return its slot number.
|* If no further Event Receiver cards are found after the starting slot, the routine will return 0.
|*
|* To locate the first Event Receiver in the crate, the routine can be called with an input
|* parameter of either 0 or 1, since 0 is not a valid slot number and slot 1 typically contains
|* the IOC processor card.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      slot = mrfFindNextEVR (lastSlot);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      lastSlot  = (epicsInt32)  Slot number of the last VME slot identified as containing
|*                                an Event Receiver card.  0, if we are just starting the search.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      slot      = (epicsInt32)  VME slot number of the next Event Receiver card to be found.
|*                                0 if no more Event Receiver cards were located in this crate.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine can be called from the vxWorks and RTEMS shells, but not from the IOC shell.
|* o The following code can be used to locate and initialize all Event Receiver cards
|*   in the crate.
|*
|*        slot = 1;
|*        while (0 != (slot = mrfFindNextEVR(slot))) {
|*            Initialize Event Receiver card at "slot";
|*        }
|*
\**************************************************************************************************/


epicsInt32 mrfFindNextEVR (epicsInt32 lastSlot)
{
    epicsInt32   slot;          /* VME slot number of next Event Receiver Card                    */
    epicsStatus  status;        /* Local status variable                                          */

   /*---------------------
    * Make sure the input parameter is in the appropriate range
    */
    if ((lastSlot > 20) || (lastSlot < 0))
        return 0;

   /*---------------------
    * Locate the next Event Receiver Card
    */
    status = vmeCRFindBoardMasked (
               lastSlot+1,              /* Start looking in the next slot to the left             */
               MRF_VME_IEEE_OUI,        /* MRF Vendor ID                                          */
               MRF_VME_EVR_BID,         /* Generic Event Receiver board ID                        */
               0xffff0000,              /* Mask out the series number                             */
               &slot);                  /* Return slot number of next card found                  */

   /*---------------------
    * Return slot number of next Event Receiver card found. 
    * Return 0 if no more Event Receiver cards were found.
    */
    if (OK == status)
         return slot;
    else return 0;

}/*end mrfFindNextEVR()*/

/**************************************************************************************************
|* mrfFindNextEVR200 () -- Locate the Next VME Slot Containing a Series 200 Event Receiver Card
|*-------------------------------------------------------------------------------------------------
|*
|* Locate the next VME slot, starting from the slot after the one specified in the input
|* parameter, which contains a series 200 Event Receiver card.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|* This routine assumes that the input parameter contains the slot number of the last
|* series 200 Event Receiver card found in the crate.  Starting with the slot after the
|* one specified in the input parameter, it searches VME CR/CSR address space looking for
|* an IEEE OUI code that matches the MRF OUI, and a Board ID code that matches the series 200
|* Event Receiver Board ID.  If a series 200 Event Receiver card is found, the routine will
|* return its slot number.  If no further series 200 Event Receiver cards are found after
|* the starting slot, the routine will return 0.
|*
|* To locate the first series 200 Event Receiver in the crate, the routine can be called with an
|* input parameter of either 0 or 1, since 0 is not a valid slot number and slot 1 typically
|* contains the IOC processor card.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      slot = mrfFindNextEVR200 (lastSlot);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      lastSlot  = (epicsInt32)  Slot number of the last VME slot identified as containing
|*                                an Event Receiver card.  0, if we are just starting the search.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      slot      = (epicsInt32)  VME slot number of the next Event Receiver card to be found.
|*                                0 if no more Event Receiver cards were located in this crate.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine can be called from the vxWorks and RTEMS shells, but not from the IOC shell.
|* o The following code can be used to locate and initialize all series 200 Event Receiver cards
|*   in the crate.
|*
|*        slot = 1;
|*        while (0 != (slot = mrfFindNextEVR200(slot))) {
|*            Initialize Event Receiver card at "slot";
|*        }
|*
\**************************************************************************************************/


epicsInt32 mrfFindNextEVR200 (epicsInt32 lastSlot)
{
    epicsInt32   slot;          /* VME slot number of next Event Receiver Card                    */

   /*---------------------
    * Make sure the input parameter is in the appropriate range
    */
    if ((lastSlot > 20) || (lastSlot < 0))
        return 0;

   /*---------------------
    * Locate the next Event Receiver Card
    * Note that both the regular Event Receiver,
    * and the Event Receiver with RF recovery have
    * the same Board ID code.
    */
    if (OK == vmeCRFindBoard (lastSlot+1, MRF_VME_IEEE_OUI, MRF_VME_EVR200RF_BID, &slot))
        return slot;

   /*---------------------
    * Return 0 if no more Event Receiver cards were found.
    */
    return 0;

}/*end mrfFindNextEVR200()*/

/**************************************************************************************************
|* mrfFindNextEVR230 () -- Locate the Next VME Slot Containing a Series 230 Event Receiver Card
|*-------------------------------------------------------------------------------------------------
|*
|* Locate the next VME slot, starting from the slot after the one specified in the input
|* parameter, which contains a series 230 Event Receiver card.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|* This routine assumes that the input parameter contains the slot number of the last
|* series 230 Event Receiver card found in the crate.  Starting with the slot after the
|* one specified in the input parameter, it searches VME CR/CSR address space looking for
|* an IEEE OUI code that matches the MRF OUI, and a Board ID code that matches the series 230
|* Event Receiver Board ID.  If a series 230 Event Receiver card is found, the routine will
|* return its slot number.  If no further series 230 Event Receiver cards are found after
|* the starting slot, the routine will return 0.
|*
|* To locate the first series 230 Event Receiver in the crate, the routine can be called with an
|* input parameter of either 0 or 1, since 0 is not a valid slot number and slot 1 typically
|* contains the IOC processor card.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      slot = mrfFindNextEVR230 (lastSlot);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      lastSlot  = (epicsInt32)  Slot number of the last VME slot identified as containing
|*                                an Event Receiver card.  0, if we are just starting the search.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      slot      = (epicsInt32)  VME slot number of the next Event Receiver card to be found.
|*                                0 if no more Event Receiver cards were located in this crate.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine can be called from the vxWorks and RTEMS shells, but not from the IOC shell.
|* o The following code can be used to locate and initialize all series 230 Event Receiver cards
|*   in the crate.
|*
|*        slot = 1;
|*        while (0 != (slot = mrfFindNextEVR230(slot))) {
|*            Initialize Event Receiver card at "slot";
|*        }
|*
\**************************************************************************************************/


epicsInt32 mrfFindNextEVR230 (epicsInt32 lastSlot)
{
    epicsInt32   slot;          /* VME slot number of next Event Receiver Card                    */

   /*---------------------
    * Make sure the input parameter is in the appropriate range
    */
    if ((lastSlot > 20) || (lastSlot < 0))
        return 0;

   /*---------------------
    * Locate the next Event Receiver Card
    * Note that both the regular Event Receiver,
    * and the Event Receiver with RF recovery have
    * the same Board ID code.
    */
    if (OK == vmeCRFindBoard (lastSlot+1, MRF_VME_IEEE_OUI, MRF_VME_EVR230RF_BID, &slot))
        return slot;

   /*---------------------
    * Return 0 if no more Event Receiver cards were found.
    */
    return 0;

}/*end mrfFindNextEVR230()*/

/**************************************************************************************************
|* mrfGetSerialNumberVME () -- Get the Serial Number of the MRF Card in the Specified Slot
|*-------------------------------------------------------------------------------------------------
|*
|* Retrieve the card's serial number from the "User CSR" area of the card's CR/CSR space
|* and format it into a printable ASCII string.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      mrfGetSerialNumberVME (Slot, SerialNumber);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Slot         = (int)     VME slot number of the card we want to obtain the
|*                               serial number for.
|*
|*-------------------------------------------------------------------------------------------------
|* OUTPUT PARAMETERS
|*      SerialNumber = (char *)  Character string, of at least MRF_SN_STRING_SIZE bytes,
|*                               which will receive the formatted serial number.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o The card serial number is the same as its MAC address.
|*
\**************************************************************************************************/


void mrfGetSerialNumberVME (epicsInt32 Slot, char *SerialNumber)
{
    epicsUInt32  *pUCSR;        /* Pointer to local buffer containing data from the User-CSR area */

   /*---------------------
    * Initialize the caller's SerialNumber string with a failure message
    * (in case we fail for some reason).
    */
    strcpy (SerialNumber, "(SN Not Found)");

   /*---------------------
    * Allocate a local buffer for the "User CSR" area of CR/CSR space.
    */
    if (NULL == (pUCSR = malloc(UCSR_SIZE)))
        return;

   /*---------------------
    * Read the User CSR space and extract the card's serial number from it
    */
    if (OK == probeUCSRSpace (Slot, pUCSR))
        getSNString (pUCSR, SerialNumber);

   /*---------------------
    * Free the "User CSR" buffer and return
    */
    free (pUCSR);
    return;

}/*end mrfGetSerialNumberVME()*/

/**************************************************************************************************
|* mrfSetAddress () -- Set VME Address and Address Modifier
|*-------------------------------------------------------------------------------------------------
|*
|* Sets the specified VME address and address modifier in the appropriate function's
|* "Address Decoder Compare Register" (ADER).  The ADER registers are located in the CSR section
|* of CR/CSR space.
|*
|* The series 200 timing cards only support function 0.  The series 230 cards support
|* function 0 for A16 access and function 1 for A24 (and higher) access.  Note that if the card
|* is mapped into A16 address space, it will not be able to use the data transmission feature.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = mrfSetAddress (Slot, VMEAddress, AddressModifier);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Slot            = (epicsInt32)  VME Slot number of the card we wish to set the address for.
|*      VMEAddress      = (epicsUInt32) The VME base address that the card should respond to.
|*      AddressModifier = (epicsUInt32) The VME address modifier that the card should respond to.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status          = (epicsStatus)  OK    if we were able to set the card's VME address.
|*                                       ERROR if we could not access CR/CSR space for the card.
|*
\**************************************************************************************************/


epicsStatus mrfSetAddress (epicsInt32 Slot,  epicsUInt32 VMEAddress, epicsUInt32 AddressModifier)
{
   /*---------------------
    * Local variables
    */
    epicsInt32    i;                    /* Local loop counter                                     */
    epicsUInt32   Function = 0;         /* Which ADER function to map the address to              */
    epicsUInt32   BoardID;              /* Board ID of the card in the specified slot             */
    epicsUInt32   ManufacturerID;       /* Manufacturer ID of the card in the specified slot      */

   /*---------------------
    * Make sure that there is an MRF timing board in the specified slot
    */
    if (OK != vmeCRGetMFG (Slot, &ManufacturerID)) return ERROR;
    if (MRF_VME_IEEE_OUI != ManufacturerID) {
        printf ("mrfSetAddress: Card in slot %d is not an MRF timing board\n", Slot);
        return ERROR;
    }/*end if card was not an MRF timing board*/

   /*---------------------
    * Always use function 0 if the address is in A16 space.
    */
    if ((AddressModifier >= 0x29) && (AddressModifier <= 0x2D))
        Function = 0;

   /*---------------------
    * If the card supports function 1, and the address modifier is not in A16 space,
    * use function 1.
    */
    else {

       /*---------------------
        * Get the board ID.  Abort on error.
        */
        if (OK != vmeCRGetBID (Slot, &BoardID))
            return ERROR;

       /*---------------------
        * Loop to see if this board supports function 1 addressing
        */
        for (i=0;  i < MRF_NUM_FUNC_ONE_CARDS;  i++) {
            if (funcOneCards[i] == BoardID) {
                Function = 1;
                break;
            }/*end if board supports function 1 addressing*/
        }/*end loop to check if board supports function 1 addressing*/

    }/*end if addressing mode is not A16*/

   /*---------------------
    * Write the address and address modifier to the proper function's ADER
    */
    return vmeCSRWriteADER (Slot, Function,
                           (VMEAddress & 0xffffff00) | ((AddressModifier & 0x3f) << 2));

}/*end mrfSetAddress()*/

/**************************************************************************************************
|* mrfSetIrqLevel () -- Set the VME Interrupt Request Level
|*-------------------------------------------------------------------------------------------------
|*
|* Sets the specified VME Interrupt Request Level (IRQ) in the appropriate field in User-Defined
|* CSR space.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = mrfSetIrqLevel (Slot, Level);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Slot   = (epicsInt32)   VME slot number of the card we wish to set the IRQ level for.
|*      Level  = (epicsInt32)   The IRQ level we wish to set.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status  = (epicsStatus)  Returns OK if we were able to set the card's IRQ level.
|*                               Returns ERROR if there was some problem setting the IRQ level.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine only works on VME Event Receiver cards, as they are the only cards capable
|*   of generating a VME interrupt.
|*
\**************************************************************************************************/


epicsStatus mrfSetIrqLevel (epicsInt32 Slot, epicsInt32 Level)
{
   /*---------------------
    * Local Variables
    */
    epicsUInt32   pUCSR;                   /* Pointer to User-CSR space (longword aligned)        */
    epicsStatus   status;                  /* Local status variable                               */
    epicsUInt32   tmp = 999;               /* Temp variable for level readback                    */

   /*---------------------
    * Make sure the requested level is in the correct range
    */
    if ((Level < 1) || (Level > 7)) {
        printf ("mrfSetIrqLevel: Invalid IRQ Level (%d).  Should be between 1 and 7\n", Level);
        return ERROR;
    }/*end if IRQ level was not valid*/

   /*---------------------
    * Try to locate the start of User-Defined CSR space
    */
    if (!(pUCSR = getUserCSRAdr (Slot))) {
        printf ("mrfSetIrqLevel: Board in slot %d does not support CR/CSR addressing\n", Slot);
        return ERROR;
    }/*end if could not access User-CSR space*/

   /*---------------------
    * Set the IRQ level in User-Defined CSR space
    */
    status = vmeCSRMemProbe ((pUCSR + UCSR_IRQ_LEVEL), CSR_WRITE, 1, (epicsUInt32 *)&Level);

   /*---------------------
    * If the write succeeded, try to read the level back to make sure it was really set.
    */
    if (OK == status)
        status = vmeCSRMemProbe ((pUCSR + UCSR_IRQ_LEVEL), CSR_READ, 1, &tmp);

   /*---------------------
    * If the read call failed, or if it read back a different value than we wrote,
    * return a failure status.
    */
    if ((OK != status) || (tmp != Level)) {
        printf ("mrfSetIrqLevel: Unable to set board in slot %d to IRQ level %d (readback: %d)\n",
                Slot, Level, tmp);
        return ERROR;
    }/*end if could not set IRQ level*/

   /*---------------------
    * If we made it this far, the IRQ level was successfully set.
    * Return a success status.
    */
    return OK;

}/*end mrfSetIrqLevel()*/

/**************************************************************************************************/
/*                      Diagnostic Routines Available Through the IOC Shell                       */
/*                                                                                                */


/**************************************************************************************************
|* mrfSNShow () -- Display the Serial Number of an MRF VME Board
|*-------------------------------------------------------------------------------------------------
|*
|* This routine will format and print the value of the board's serial number.  The serial number
|* is displayed as a six-byte internet MAC address (which is what it is).
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      mrfSNShow (Slot);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Slot  = (epicsInt32)  VME slot number of the card we wish to display
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o To initially set the IP address of an MRF Series 200 card, you need to know the card's
|*   MAC address so that you can use "arp/ping".  The MAC address will be on a lable on the
|*   front of the card, but if the lable falls off, or becomes unreadable, you can also get
|*   the MAC address by calling mrfSNShow on the card's VME slot.
|*
\**************************************************************************************************/


void mrfSNShow (epicsInt32 Slot)
{
   /*---------------------
    * Local variables
    */
    epicsUInt32  *pUCSR;                                /* Pointer to local User-CSR buffer       */
    char          serialNumber [MRF_SN_STRING_SIZE];    /* Formatted serial number                */

   /*---------------------
    * Allocate a local memory buffer to read User-CSR space into.
    */
    if (NULL == (pUCSR = calloc(1, UCSR_SIZE)))
        return;

   /*---------------------
    * Read a copy of User-CSR space into the local buffer.
    * If successful, extract, format, and display the serial number.
    */
    if (OK == probeUCSRSpace(Slot, pUCSR)) {
        getSNString (pUCSR, serialNumber);
        printf("Board serial number: %s\n", serialNumber);
    }/*end if found User-CSR space*/

   /*---------------------
    * Print an error if we could not access CR/CSR space for this slot.
    */
    else
        printf ("No VME64 CSR capable module in slot %d.\n", Slot);

   /*---------------------
    * Free the local buffer and return.
    */
    free (pUCSR);

}/*end mrfSNShow()*/

/**************************************************************************************************
|* vmeCRShow () -- Display the Contents of the VME Configuration ROM (CR)
|*-------------------------------------------------------------------------------------------------
|*
|* This routine will format and print the contents of a VME-64X card's configuration ROM.
|*
|* All values are displayed in hexadecimal, so some familiarity with the format of VME 64
|* configuration ROM space will be required to interpret the output.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      vmeCRShow (Slot);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Slot  = (epicsInt32)  VME slot number of the card we wish to display
|*
\**************************************************************************************************/


void vmeCRShow (epicsInt32 Slot)
{
   /*---------------------
    * Local variables
    */
    epicsUInt32   i,j;          /* Local loop counters                                            */
    epicsUInt32  *pCR;          /* Pointer to local in-memory copy of the card's CR space         */

   /*---------------------
    * Allocate a memory buffer for the card's CR space.
    * Abort on error.
    */
    if (NULL == (pCR = malloc(CR_SIZE))) {
        printf ("Unable to allocate memory for displaying CR space.");
        return;
    }/*end if could not allocate memory*/

   /*---------------------
    * Read in the configuration ROM for the card at this slot.
    * Abort on error.
    */
    if (OK != probeCRSpace(Slot, pCR)) {
        printf ("No CR/CSR capable board in slot %d found.\n", Slot);
        free(pCR);
        return;
    }/*end if could not read the card's configuration ROM*/

   /*---------------------
    * Display the configuration ROM checksum.
    */
    printf ("CR Area for Slot %d\n", Slot);
    printf ("  CR Checksum: 0x%02x\n", pCR[LINDEX(CR_ROM_CHECKSUM)]);

   /*---------------------
    * Display the number of bytes in the configuration ROM.
    */
    printf ("  ROM Length:   0x%02x%02x%02x\n",
            pCR[LINDEX(CR_ROM_LENGTH) + 0],
            pCR[LINDEX(CR_ROM_LENGTH) + 1],
            pCR[LINDEX(CR_ROM_LENGTH) + 2]);

   /*---------------------
    * Display the configuration ROM area (CR) access width
    */
    printf ("  CR data access width: 0x%02x\n", pCR[LINDEX(CR_DATA_ACCESS_WIDTH)]);

   /*---------------------
    * Display the Control/Status Register area (CSR) access width and space specification.
    */
    printf ("  CSR data access width: 0x%02x\n", pCR[LINDEX(CSR_DATA_ACCESS_WIDTH)]);
    printf ("  CR/CSR Space Specification ID: 0x%02x\n", pCR[LINDEX(CR_SPACE_ID)]);

   /*---------------------
    * Display the manufacturer's IEEE ID number
    */
    printf ("  Manufacturer's ID %02x-%02x-%02x\n",
            pCR[LINDEX(CR_IEEE_OUI) + 0],
            pCR[LINDEX(CR_IEEE_OUI) + 1],
            pCR[LINDEX(CR_IEEE_OUI) + 2]);

   /*---------------------
    * Display the board ID number
    */
    printf ("  Board ID 0x%02x%02x%02x%02x\n",
            pCR[LINDEX(CR_BOARD_ID) + 0],
            pCR[LINDEX(CR_BOARD_ID) + 1],
            pCR[LINDEX(CR_BOARD_ID) + 2],
            pCR[LINDEX(CR_BOARD_ID) + 3]);

   /*---------------------
    * Display the board revision number
    */
    printf ("  Revision ID 0x%02x%02x%02x%02x\n",
            pCR[LINDEX(CR_REVISION_ID) + 0],
            pCR[LINDEX(CR_REVISION_ID) + 1],
            pCR[LINDEX(CR_REVISION_ID) + 2],
            pCR[LINDEX(CR_REVISION_ID) + 3]);

   /*---------------------
    * Display the offset to the manufacturer-specific ASCII string
    */
    printf ("  Offset to ASCII string: 0x%02x%02x%02x\n",
            pCR[LINDEX(CR_ASCII_STRING) + 2],
            pCR[LINDEX(CR_ASCII_STRING) + 1],
            pCR[LINDEX(CR_ASCII_STRING) + 0]);

   /*---------------------
    * Display the Program ID code
    */
    printf ("  Program ID: 0x%02x\n", pCR[LINDEX(CR_PROGRAM_ID)]);

   /*---------------------
    * Display the start and end of the manufacturer-specific CR space
    */
    printf ("  Start of USER_CR: 0x%02x%02x%02x\n",
            pCR[LINDEX(CR_BEG_UCR) + 2],
            pCR[LINDEX(CR_BEG_UCR) + 1],
            pCR[LINDEX(CR_BEG_UCR) + 0]);
    printf ("  End of USER_CR:   0x%02x%02x%02x\n",
            pCR[LINDEX(CR_END_UCR) + 2],
            pCR[LINDEX(CR_END_UCR) + 1],
            pCR[LINDEX(CR_END_UCR) + 0]);

   /*---------------------
    * Display the start and end of the manufacturer-specific configuration RAM space
    */
    printf ("  Start of CRAM: 0x%02x%02x%02x\n",
            pCR[LINDEX(CR_BEG_CRAM) + 2],
            pCR[LINDEX(CR_BEG_CRAM) + 1],
            pCR[LINDEX(CR_BEG_CRAM) + 0]);
    printf ("  End of CRAM:   0x%02x%02x%02x\n",
            pCR[LINDEX(CR_END_CRAM) + 2],
            pCR[LINDEX(CR_END_CRAM) + 1],
            pCR[LINDEX(CR_END_CRAM) + 0]);

   /*---------------------
    * Display the start and end of the manufacturer-specific CSR space
    */
    printf ("  Start of USER_CSR: 0x%02x%02x%02x\n",
            pCR[LINDEX(CR_BEG_UCSR) + 2],
            pCR[LINDEX(CR_BEG_UCSR) + 1],
            pCR[LINDEX(CR_BEG_UCSR) + 0]);
    printf ("  End of USER_CSR:   0x%02x%02x%02x\n",
            pCR[LINDEX(CR_END_UCSR) + 2],
            pCR[LINDEX(CR_END_UCSR) + 1],
            pCR[LINDEX(CR_END_UCSR) + 0]);

   /*---------------------
    * Display the start and end of the card's serial number area.
    */
    printf ("  Start of Serial Number: 0x%02x%02x%02x\n",
            pCR[LINDEX(CR_BEG_SN) + 2],
            pCR[LINDEX(CR_BEG_SN) + 1],
            pCR[LINDEX(CR_BEG_SN) + 0]);
    printf ("  End of Serial Number:   0x%02x%02x%02x\n",
            pCR[LINDEX(CR_END_SN) + 2],
            pCR[LINDEX(CR_END_SN) + 1],
            pCR[LINDEX(CR_END_SN) + 0]);

   /*---------------------
    * Display the VME slave-mode characteristics
    */
    printf ("  Slave Characteristics Parameter: 0x%02x\n", pCR[LINDEX(CR_SLAVE_CHAR)]);
    printf ("  User-defined Slave Characteristics: 0x%02x\n", pCR[LINDEX(CR_UD_SLAVE_CHAR)]);

   /*---------------------
    * Display the VME master-mode characteristics
    */
    printf ("  Master Characteristics Parameter: 0x%02x\n", pCR[LINDEX(CR_MASTER_CHAR)]);
    printf ("  User-defined Master Characteristics: 0x%02x\n", pCR[LINDEX(CR_UD_MASTER_CHAR)]);

   /*---------------------
    * Display the interrupt capabilities of the card
    */
    printf ("  Interrupt Handler Capabilities: 0x%02x\n", pCR[LINDEX(CR_IRQ_HANDLER_CAP)]);
    printf ("  Interrupter Capabilities: 0x%02x\n", pCR[LINDEX(CR_IRQ_CAP)]);

   /*---------------------
    * Display the configuration RAM area (CRAM) access width
    */
    printf ("  CRAM Data Access Width: 0x%02x\n", pCR[LINDEX(CR_CRAM_WIDTH)]);

   /*---------------------
    * Display the Data Access Width Parameter (DAWPR) for each of the card's functions
    */
    for (i = 0; i < VME_64X_NUM_FUNCTIONS; i++) {
        printf ("  Function %d Data Access Width 0x%02x\n", i,
                pCR[LINDEX(CR_FN_DAWPR) + (i * CR_DAWPR_BYTES)]);
    }/*end for each function*/

   /*---------------------
    * Display the VME Address Mode Capability Mask (AMCAP) for each of the card's functions
    */
    for (i = 0; i < VME_64X_NUM_FUNCTIONS; i++) {
        printf ("  Function %d AM Code Mask: 0x", i);
        for (j = 0; j < CR_AMCAP_BYTES; j++)
            printf ("%02x", pCR[LINDEX(CR_FN_AMCAP) + ((i * CR_AMCAP_BYTES) + j)]);
        printf ("\n");
    }/*end for each function*/

   /*---------------------
    * Display the VME Extended Address Mode Capability Mask (XAMCAP)
    * for each of the card's functions.
    */
    for (i = 0; i < VME_64X_NUM_FUNCTIONS; i++) {
        printf ("  Function %d XAM Code Mask: \n    0x", i);
        for (j = 0; j < CR_XAMCAP_BYTES; j++)
            printf ("%02x", pCR[LINDEX(CR_FN_XAMCAP) + ((i * CR_XAMCAP_BYTES) + j)]);
        printf ("\n");
    }/*end for each function*/

   /*---------------------
    * Display the Address Decoder Mask (ADEM) for each of the card's functions
    */
    for (i = 0; i < VME_64X_NUM_FUNCTIONS; i++) {
        printf ("  Function %d Address Decoder Mask: 0x", i);
        for (j = 0; j < CR_ADEM_BYTES; j++)
            printf ("%02x", pCR[LINDEX(CR_FN_ADEM) + ((i * CR_ADEM_BYTES) + j)]);
        printf ("\n");
    }/*end for each function*/

   /*---------------------
    * Display the Master DAWPR, AMCAP, and XAMCAP values
    */
    printf ("  Master Data Access Width: 0x%02x\n", pCR[LINDEX(CR_MASTER_DAWPR)]);

    printf ("  Master AM Capability: 0x");
    for (j = 0; j < CR_AMCAP_BYTES;  j++)
        printf ("%02x", pCR[LINDEX(CR_MASTER_AMCAP) + j]);
    printf ("\n");

    printf ("  Master XAM Capability: \n    0x");
    for (j = 0; j < CR_XAMCAP_BYTES;  j++)
        printf ("%02x", pCR[LINDEX(CR_MASTER_XAMCAP) + j]);
    printf ("\n");

   /*---------------------
    * Release the local memory and return.
    */
    free (pCR);

}/*end vmeCRShow()*/

/**************************************************************************************************
|* vmeCSRShow () -- Display the Contents of the VME Control/Status Register Area (CSR)
|*-------------------------------------------------------------------------------------------------
|*
|* This routine will format and print the contents of a VME-64X card's Control/Status Register
|* area.  The contents of the User CSR area are also displayed.
|*
|* All values are displayed in hexadecimal, so some familiarity with the format of VME 64
|* CSR space will be required to interpret the output.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      vmeCSRShow (Slot);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Slot  = (epicsInt32)  VME slot number of the card we wish to display
|*
\**************************************************************************************************/


void vmeCSRShow (epicsInt32 Slot)
{
   /*---------------------
    * Local variables
    */
    epicsUInt32   array[4];             /* Local array for unpacking multi-byte values            */
    epicsUInt32   byte;                 /* Local value for single-byte values                     */
    epicsInt32    i, j;                 /* Local loop counters                                    */
    epicsUInt32   offsetCR;             /* Address of CR/CSR space for the desired slot           */
    epicsUInt32   offsetFunc;           /* Address of the first function ADER register            */

    offsetCR = CSR_BASE (Slot);

   /*---------------------
    * Check that CR/CSR capable board present at slot
    */
    if (OK != vmeCSRMemProbe (offsetCR, CSR_READ, 1, &byte)) {
        printf("No VME64 CSR capable module in slot %d.\n", Slot);
        return;
    }/*end if board does not support CR/CSR addressing*/

   /*---------------------
    * Display the CR/CSR space address, as contained in the Base Address (BAR) register.
    */
    vmeCSRMemProbe ((offsetCR + CSR_BAR), CSR_READ, 1, &byte);
    printf ("CSR Area for Slot %d\n", Slot);
    printf ("  CR/CSR Base Address (BAR): 0x%06X\n", (byte << 16));

   /*---------------------
    * Display the contents of the "Bit Set" (status) register
    */
    vmeCSRMemProbe ((offsetCR + CSR_BIT_SET), CSR_READ, 1, &byte);
    printf ("  Bit Set Register Status: 0x%02X\n", byte);

    printf ("    * module %sin reset mode\n",
                  (byte & CSR_BITSET_RESET_MODE) ? "" : "not ");
    printf ("    * SYSFAIL driver %s\n",
                  (byte & CSR_BITSET_SYSFAIL_ENA) ? "enabled" : "disabled");
    printf ("    * module %sfailed\n",
                  (byte & CSR_BITSET_MODULE_FAIL) ? "" : "not ");
    printf ("    * module %s\n",
                  (byte & CSR_BITSET_MODULE_ENA) ? "enabled" : "disabled");
    printf ("    * module %s BERR*\n",
                  (byte & CSR_BITSET_BERR) ? "issued" : "did not issue");
    printf ("    * CRAM %sowned\n",
                  (byte & CSR_BITSET_CRAM_OWNED) ? "" : "not ");

   /*---------------------
    * Display the value of the CRAM Owner register
    */
    vmeCSRMemProbe ((offsetCR + CSR_CRAM_OWNER), CSR_READ, 1, &byte);
    printf("  CRAM owner: 0x%02x\n", byte);


   /*---------------------
    * Loop to display the ADER registers for each function
    */
    for (i=0;  i < 8;  i++) {
        printf("  Function %d ADER: 0x", i);

        offsetFunc = offsetCR + CSR_FN0_ADER + (i * CSR_ADER_SIZE);
        vmeCSRMemProbe (offsetFunc, CSR_READ, CSR_ADER_BYTES, array);

        for (j=0;  j < CSR_ADER_BYTES;  j++)
            printf ("%02X", array[j]);

        printf ("\n");

    }/*end for each defined function*/

   /*---------------------
    * Display the contents of the User CSR area
    */
    printf ("\n");
    vmeUserCSRShow (Slot);

}/*end vmeCRShow()*/

/**************************************************************************************************
|* vmeUserCSRShow () -- Display the Contents of the User-CSR Area
|*-------------------------------------------------------------------------------------------------
|*
|* This routine will format and print the contents of a VME-64X card's User-CSR area.
|* The contents of the User-CSR area are card and vendor specific.  For MRF series 200
|* cards, the User-CSR area contains:
|*   o The card's network MAC address (which also serves as the serial number)
|*   o The VME IRQ level (Event Receiver cards only)
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      vmeUserCSRShow (Slot);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Slot  = (epicsInt32)  VME slot number of the card we wish to display
|*
\**************************************************************************************************/


void vmeUserCSRShow (epicsInt32 Slot)
{
   /*---------------------
    * Local variables
    */
    epicsUInt32  *pUCSR;                                /* Address of local memory buffer         */
    char          serialNumber [MRF_SN_STRING_SIZE];    /* String for formatted serial number     */

   /*---------------------
    * Allocate a block of memory to hold the contents of User-CSR space.
    * Abort on failure.
    */
    if (NULL == (pUCSR = malloc(UCSR_SIZE)))
        return;

   /*---------------------
    * Read and display the contents of User-CSR space into the local memory.
    */
    if (OK == probeUCSRSpace (Slot, pUCSR)) {
        printf ("User CSR Area for Slot %d\n", Slot);

       /*---------------------
        * Display the board's serial number (MAC address)
        */
        getSNString (pUCSR, serialNumber);
        printf ("  Board serial number: %s\n", serialNumber);

       /*---------------------
        * Display the IRQ level
        */
        printf ("  IRQ Level: %d\n", pUCSR[LINDEX(UCSR_IRQ_LEVEL)]);

    }/*end if we could read the User-CSR space*/

   /*---------------------
    * If we could not access User-CSR space for this slot, print out an error message.
    */
    else printf ("No VME64 CSR capable module in slot %d.\n", Slot);

   /*---------------------
    * Free the local memory buffer and exit.
    */
    free (pUCSR);

}/*end vmeUserCSRShow()*/

/**************************************************************************************************/
/*                              Global Utility Routines                                           */
/*                                                                                                */


/**************************************************************************************************
|* vmeCRFindBoard () -- Locate the Next VME Slot Containing the Desired Board
|*-------------------------------------------------------------------------------------------------
|*
|* Starting at the specified VME slot number, locate the first VME card which contains the
|* specified manufacturer and device ID numbers in it's configuration space.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|* This routine calls the vmeCRFindBoardMasked() routine with a mask of all ones, which will
|* force the board ID match to be exact.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = vmeCRFindBoard (LastSlot, ManufacturerID, BoardID, &Slot);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      StartSlot      = (epicsInt32)  The VME slot number at which we should start the search
|*                                     for a board with the desired manufacturer and board ID.
|*      ManufacturerID = (epicsUInt32) The IEEE  OUI (Organizational Unique Identifier) code
|*                                     to look for.
|*      BoardID        = (epicsUInt32) The manufacturer-specific board ID to look for..
|*
|*-------------------------------------------------------------------------------------------------
|* OUTPUT PARAMETERS:
|*      Slot           = (epicsInt32 *) The slot number of the first VME board we found,
|*                                      starting at StartSlot, that has the correct manufacturer
|*                                      and board ID.  If no such board was located, return 0.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status         = (epicsStatus)  Returns OK if we were able to locate a board with the
|*                                      desired manufacturer and boar ID.
|*                                      Otherwise returns ERROR.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This is a legacy routine, preserved here for compatability.
|*
\**************************************************************************************************/


epicsStatus vmeCRFindBoard (
    epicsInt32    StartSlot,            /* The VME slot number where we should start the search   */
    epicsUInt32   ManufacturerID,       /* Manufacturer ID of the board we are looking for        */
    epicsUInt32   BoardID,              /* Board ID of the type of board we are looking for       */
    epicsInt32   *Slot)                 /* Slot number of the board we are looking for (if found) */
{
    return vmeCRFindBoardMasked (StartSlot, ManufacturerID, BoardID, 0xffffffff, Slot);

}/*end vmeCRFindBoard()*/

/**************************************************************************************************
|* vmeCRFindBoardMasked () -- Locate the Next VME Slot Containing the Desired Board
|*-------------------------------------------------------------------------------------------------
|*
|* Starting at the specified VME slot number, locate the first VME card which contains the
|* specified manufacturer and (masked) device ID numbers in it's configuration space.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|* This routine functions exactly like the vmeCRFindBoard() routine, except that a mask word is
|* anded with the desired and current board ID values before the comparison.  This allows the
|* routine to 
|* Starting with the specified slot number, examine each VME board we find that supports
|* CR/CSR space.  When we find a board that supports CR/CSR space, extract the manufacturer's
|* IEEE OUI (Organizationally Unique Identifier) and the board ID from CR-Space.  A mask is
|* applied to the board IDs before comparison so that we can search for generic board types as
|* well as specific boards.  Examples of generic board searches might include  all Event Receiver
|* boards regardless of the series, or all series 230 boards.
|*
|* If the manufacturer and board IDs match the ones we want, terminate the search and return the
|* slot number of the board we found.  If we did not find a board with matching manufacturer
|* and board ID's, return an error status and set the slot number to 0.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = vmeCRFindBoardMasked (LastSlot, ManufacturerID, BoardID, Mask, &Slot);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      StartSlot      = (epicsInt32)  The VME slot number at which we should start the search
|*                                     for a board with the desired manufacturer and board ID.
|*      ManufacturerID = (epicsUInt32) The IEEE  OUI (Organizational Unique Identifier) code
|*                                     to look for.
|*      BoardID        = (epicsUInt32) The manufacturer-specific board ID to look for.
|*      Mask           = (epicsUInt32) Mask to be used when comparomg Board IDs. 
|*
|*-------------------------------------------------------------------------------------------------
|* OUTPUT PARAMETERS:
|*      Slot           = (epicsInt32 *) The slot number of the first VME board we found,
|*                                      starting at StartSlot, that has the correct manufacturer
|*                                      and board ID.  If no such board was located, return 0.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status         = (epicsStatus)  Returns OK if we were able to locate a board with the
|*                                      desired manufacturer and boar ID.
|*                                      Otherwise returns ERROR.
|*
\**************************************************************************************************/


epicsStatus vmeCRFindBoardMasked (
    epicsInt32    StartSlot,            /* The VME slot number where we should start the search   */
    epicsUInt32   ManufacturerID,       /* Manufacturer ID of the board we are looking for        */
    epicsUInt32   BoardID,              /* Board ID of the type of board we are looking for       */
    epicsUInt32   Mask,                 /* Mask to use when comparing board ID values             */
    epicsInt32   *Slot)                 /* Slot number of the board we are looking for (if found) */
{
   /*---------------------
    * Local variables
    */
    epicsUInt32   CurrentBoardID;       /* Board's board ID, as read from CR space                */
    epicsUInt32   CurrentMfgID;         /* Board's manufacturer ID, as read from CR space         */
    epicsInt32    CurrentSlot;          /* VME slot number we are currently checking              */
    epicsStatus   status;               /* Local status variable                                  */

   /*---------------------
    * Loop to check each remaining slot for a board with the desired manufacturer
    * and device ID.
    */
    for (CurrentSlot = StartSlot;  CurrentSlot <= MRF_MAX_VME_SLOT;  CurrentSlot++) {

       /*---------------------
        * Get the manufacturer's OUI for the card in this slot
        */
        status = vmeCRGetMFG (CurrentSlot, &CurrentMfgID);

       /*---------------------
        * Skip this slot if we could not read the manufacturer ID,
        * or if it did not match the manufacturer ID we are looking for.
        */
        if (OK != status) continue;
        if (CurrentMfgID != ManufacturerID) continue;

       /*---------------------
        * Extract the manufacturer's board ID for the card in this slot
        */
        status = vmeCRGetBID (CurrentSlot, &CurrentBoardID);
        if (OK != status) continue;

       /*---------------------
        * If the manufacturer and board ID's match, return a success code
        * and the slot number of the board we found.
        */
        if ((CurrentBoardID & Mask) == (BoardID & Mask)) {
            *Slot = CurrentSlot;
            return OK;
        }/*end if found the desired manufacturer and board id*/

    }/*end for each remaining slot*/

   /*---------------------
    * If we got out of the loop without a manufacturer and board ID match,
    * return a failure code.
    */
    return ERROR;

}/*end vmeCRFindBoardMasked()*/

/**************************************************************************************************
|* vmeCRGetBID () -- Get the Board ID of the Card in the Specified Slot
|*-------------------------------------------------------------------------------------------------
|*
|* Retrieves the card's board ID from the "CR" area of the card's CR/CSR space
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = vmeCRGetBID (Slot, &BoardID);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Slot     = (epicsInt32)     VME slot number of the card we want to obtain the Board ID for.
|*
|*-------------------------------------------------------------------------------------------------
|* OUTPUT PARAMETERS
|*      BoardID  = (epicsUInt32 *)  Receives the Board ID
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status   = (epicsStatus)    Returns OK if we were able to read the Board ID from CR space
|*                                  Returns ERROR if there was no CR/CSR capable board at the
|*                                  specified slot.
|*
\**************************************************************************************************/


epicsStatus vmeCRGetBID (epicsInt32 Slot, epicsUInt32 *BoardID)
{
   /*---------------------
    * Local variables
    */
    epicsUInt32   Buffer [CR_BOARD_ID_BYTES]; /* Local buffer for Board ID value                  */
    epicsUInt32   offsetCR;                   /* Address of CR/CSR space for the current slot     */
    epicsStatus   status;                     /* Local status variable                            */

   /*---------------------
    * Initialization:
    *   Get the offset to CR/CSR space for this slot.
    *   Clear the Board ID return value.
    */
    offsetCR = CSR_BASE (Slot);
    *BoardID = 0;

   /*---------------------
    * Extract the manufacturer's board ID from CR space
    */
    status = vmeCSRMemProbe ((offsetCR + CR_BOARD_ID),
                             CSR_READ,
                             CR_BOARD_ID_BYTES,
                             Buffer);

   /*---------------------
    * Abort if we could not read CR space
    */
    if (OK != status)
        return ERROR;

   /*---------------------
    * Convert the Board ID bytes to a longword and return
    */
    convCRtoLong (Buffer, CR_BOARD_ID_BYTES, BoardID);
    return OK;

}/*end vmeCRGetBID()*/

/**************************************************************************************************
|* vmeCRGetMFG () -- Get the Manufacturer ID of the Card in the Specified Slot
|*-------------------------------------------------------------------------------------------------
|*
|* Retrieves the card's manufacturer ID from the "CR" area of the card's CR/CSR space
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = vmeCRGetMFG (Slot, &MfgID);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Slot     = (epicsInt32)     VME slot number of the card we want to obtain the ID for.
|*
|*-------------------------------------------------------------------------------------------------
|* OUTPUT PARAMETERS
|*      MfgID    = (epicsUInt32 *)  Receives the manufacturer ID
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status   = (epicsStatus)    Returns OK if we were able to read the manufacturer ID from
|*                                  CR space.
|*                                  Returns ERROR if there was no CR/CSR capable board at the
|*                                  specified slot.
|*
\**************************************************************************************************/


epicsStatus vmeCRGetMFG (epicsInt32 Slot, epicsUInt32 *MfgID)
{
   /*---------------------
    * Local variables
    */
    epicsUInt32   Buffer [CR_IEEE_OUI_BYTES]; /* Local buffer for manufacturer ID value           */
    epicsUInt32   offsetCR;                   /* Address of CR/CSR space for the current slot     */
    epicsStatus   status;                     /* Local status variable                            */

   /*---------------------
    * Initialization:
    *   Get the offset to CR/CSR space for this slot.
    *   Clear the manufacturer ID.
    */
    offsetCR = CSR_BASE (Slot);
    *MfgID = 0;

   /*---------------------
    * Extract the manufacturer's IEEE ID from CR space
    */
    status = vmeCSRMemProbe ((offsetCR + CR_IEEE_OUI),
                             CSR_READ,
                             CR_IEEE_OUI_BYTES,
                             Buffer);

   /*---------------------
    * Abort if we could not read CR space
    */
    if (OK != status)
        return ERROR;

   /*---------------------
    * Convert the manufacturer ID bytes to a longword and return
    */
    convCRtoLong (Buffer, CR_IEEE_OUI_BYTES, MfgID);
    return OK;

}/*end vmeCRGetMFG()*/

/**************************************************************************************************
|* vmeCSRWriteADER () -- Write the VME Address Decoder (ADER) Register
|*-------------------------------------------------------------------------------------------------
|*
|* Writes the VME base address and address modifier to the specified slot and specified function's
|* Address Decoder register (ADER).
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = vmeCSRWriteADER (Slot, Func, Value);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Slot           = (epicsInt32)  The VME slot number
|*      Func           = (epicsUInt32) The VME function number.
|*      Value          = (epicsUInt32) The value to write to the function's ADER register.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status         = (epicsStatus)  Returns OK if all the parameters were in range and
|*                                      we were able to successfully write to the specified
|*                                      function's ADER register.
|*                                      Otherwise returns ERROR.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This routine assumes that both the base address and the address mode are contained
|*   in the appropriate fields of the Value parameter.
|*
\**************************************************************************************************/


epicsStatus vmeCSRWriteADER (epicsInt32 Slot, epicsUInt32 Func, epicsUInt32 Value)
{
   /*---------------------
    * Local variables
    */
    epicsUInt32   offsetCR;                     /* Address in CR/CSR space of the ADER register   */
    epicsUInt32   tmp [CSR_ADER_BYTES];         /* Temp array for constructing the ADER value     */
    epicsInt32    i;                            /* Local loop counter                             */

   /*---------------------
    * Make sure the specified function is in range.
    */
    if (Func >= VME_64X_NUM_FUNCTIONS) {
        printf ("vmeCSRWriteADER: Invalid function number (%d).\n", Func);
        printf ("                 Must be between 0 and %u.\n", VME_64X_NUM_FUNCTIONS);
        return ERROR;
    }/*end if function number was invalid*/

   /*---------------------
    * Calculate the address in CR/CSR space of the ADER register for the specified function.
    */
    offsetCR = CSR_BASE(Slot) + CSR_FN0_ADER + (Func * CSR_ADER_SIZE);

   /*---------------------
    * Separate the desired ADER value into an array of bytes, most significant byte first.
    */
    for (i = 0; i < CSR_ADER_BYTES; i++)
        tmp[i] = (Value >> (8 * (3 - i))) & 0xff;

   /*---------------------
    * Attempt to write the desired value to the ADER register and
    * return whether or not we succeeded.
    */
    return vmeCSRMemProbe (offsetCR, CSR_WRITE, CSR_ADER_BYTES, tmp);

}/*end vmeCSRWriteADER()*/

/**************************************************************************************************/
/*                               Local Utility Routines                                           */
/*                                                                                                */


/**************************************************************************************************
|* convCRtoLong () -- Convert a value in CR/CSR space to a Long Integer
|*-------------------------------------------------------------------------------------------------
|*
|* Converts values in CR/CSR space (up to 4 bytes) from CR/CSR-Space format (every fourth byte)
|* to long integer format.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = convCRtoLong (pCR, NumBytes, &Value);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCR         = (epicsUInt32 *)  Pointer to buffer containing data read from CR/CSR space
|*      NumBytes    = (epicsInt32)     Number of bytes in value to convert
|*
|*-------------------------------------------------------------------------------------------------
|* OUTUT PARAMETERS:
|*      Value       = (epicsUInt32 *)  Pointer to long integer to receive the converted value.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status      = (epicsStatus)     OK if conversion was successful.
|*                                      ERROR if the byte count was invalid.
|*
\**************************************************************************************************/

static
epicsStatus convCRtoLong (epicsUInt32 *pCR, epicsInt32 NumBytes, epicsUInt32 *Value)
{
   /*---------------------
    * Local variables
    */
    epicsInt32    i;            /* Local loop counter                                             */
    epicsUInt32   val = 0;      /* Assembled value                                                */

   /*---------------------
    * Check for range errors
    */
    if (NumBytes < 1 || NumBytes > 4) {
        printf ("convCRToLong(): Invalid number of bytes (%d).  Should be between 1 and 4\n",
                NumBytes);
        return ERROR;
    }/*end if number of bytes was out of range*/

   /*---------------------
    * Convert the offset bytes to a long integer
    */
    for (i=0;  i < NumBytes;  i++) {
        val <<= 8;
        val |= pCR[i] & 0xff;
    }/*end for each byte in the CR/CSR register*/

   /*---------------------
    * Return the converted value
    */
    *Value = val;
    return OK;

}/*end convCRtoLong()*/

/**************************************************************************************************
|* convOffsetToLong () -- Convert an Offset Value to a Long Integer
|*-------------------------------------------------------------------------------------------------
|*
|* Converts offset values in CR space (up to 4 bytes) from CR/CSR-Space format (every fourth byte)
|* to long integer format.
|*
|* This routine is similar to convCRtoLong(), except that offsets are stored
|* in "little-endian" format.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = convOffsetToLong (pCR, NumBytes, &Value);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pCR         = (epicsUInt32 *)  Points to first long integer in the offset array
|*      NumBytes    = (epicsInt32)     Number of bytes in the offset value (usually 3)
|*
|*-------------------------------------------------------------------------------------------------
|* OUTUT PARAMETERS:
|*      Value       = (epicsUInt32 *)  Pointer to long integer to receive the converted value.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status      = (epicsStatus)     OK if conversion was successful.
|*                                      ERROR if the byte count was invalid.
|*
\**************************************************************************************************/

static
epicsStatus convOffsetToLong (epicsUInt32 *pCR, epicsInt32 NumBytes, epicsUInt32 *Value)
{
   /*---------------------
    * Local variables
    */
    epicsInt32    i;            /* Local loop counter                                             */
    epicsUInt32   val = 0;      /* Assembled value                                                */

   /*---------------------
    * Check for range errors
    */
    if (NumBytes < 1 || NumBytes > 4) {
        printf ("convOffsetToLong(): Invalid number of bytes (%d).  Should be between 1 and 4\n",
                NumBytes);
        return ERROR;
    }/*end if number of bytes was out of range*/

   /*---------------------
    * Assemble the bytes in the offset array into a long integer
    * using little-endian ordering.
    */
    for (i = NumBytes-1;  i >= 0;  i--) {
        val <<= 8;
        val |= pCR[i] & 0xff;
    }/*end for each byte (in descending order)*/

   /*---------------------
    * Return the converted value
    */
    *Value = val;
    return OK;

}/*end convOffsetToLong()*/

/**************************************************************************************************
|* getSNString () -- Extract and Format the Board's Serial Number
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   o Extracts the MRF Event Receiver or Event Generator board's serial number from a buffer
|*     containing the contents of User-CSR space.
|*   o Formats the serial number into an ASCII string representing the serial number as a
|*     six-byte, hexadecimal value formatted to look like an internet MAC address (which is
|*     what it is really).
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      getSNString (pUCSR, stringValue);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      pUCSR      = (epicsUInt32 *) Address of a local memory buffer containing the contents
|*                                   of the User-CSR space for the board in question.
|*
|*-------------------------------------------------------------------------------------------------
|* OUTUT PARAMETERS:
|*      stringValue = (char *)       Address of a memory buffer that will contain the formatted
|*                                   serial number string.
|*
\**************************************************************************************************/

static
void getSNString (epicsUInt32 *pUCSR, char *stringValue)
{
   /*---------------------
    * Local variables
    */
    epicsUInt8   byte;          /* Value of the current serial number byte we are processing      */
    epicsInt32   i;             /* Local loop counter                                             */
    char        *pChar;         /* Pointer to the next character in the output string             */

   /*---------------------
    * For each byte in the serial number, convert the byte to a hexadecimal character string.
    * Separate each byte by a colon (:) character.
    */
    pChar = stringValue;
    for (i=0;  i < MRF_SN_BYTES;  i++) {
        byte = pUCSR[LINDEX(UCSR_SERIAL_NUMBER) + i] & 0xff;
        if (i) *pChar++ = ':';
        *pChar++ = hexChar[byte >> 4];
        *pChar++ = hexChar[byte & 0xf];
    }/*end for each byte in the serial number*/

   /*---------------------
    * Add a terminator to the end of the output string and exit.
    */
    *pChar = '\0';

}/*end getSNString()*/

/**************************************************************************************************
|* getUserCSRAdr () -- Get the CR/CSR Address of the User-CSR Area
|*-------------------------------------------------------------------------------------------------
|*
|* For the specified VME slot number, this routine will locate the offset (contained in CR space)
|* to the User-CSR space.  It will then compute the address, in CR/CSR space of the User-CSR space
|* and return this address as a pointer to an MRF_USR_CSR structure.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    o Compute the CR/CSR address of the field in CR space that contains the offset to
|*      the start of User-CSR space (CR_BEG_UCSR).
|*    o Read the value of the "CR_BEG_UCSR" field into local memory.
|*    o Convert the value in the "CR_BEG_UCSR" field from CR/CSR format (every fourth byte)
|*      to a long integer format.
|*    o If the offset to the start of User-CSR space was not found at the designated offset
|*      in CR space, use the default offset for MRF series 200 cards.
|*    o Compute and return the address of the start of User-CSR space by adding the offset
|*      computed above to the base CR/CSR address for the specified VME slot and forcing
|*      the result to be longword aligned.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      pUserCSR = getUserCSRAdr (slot);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Slot      = (epicsInt32)   VME slot number of the card we want to get the User-CSR
|*                                 address for.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      pUserCSR  = (epicsUInt32)  Address, in CR/CSR space, of the start of User-CSR space.
|*                                 Returns 0 if the card in the specified slot does not support
|*                                 CR/CSR addressing.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o The address returned is forced to be longword aligned.   This way the defined UCSR offsets
|*   will give you the correct VME addresses when added to the address we return.
|* o The MRF Series 200 VME cards store the offset values in little-endian order.  The routine,
|*   convOffsetToLong() corrects for this.
|*
\**************************************************************************************************/

static
epicsUInt32 getUserCSRAdr (epicsInt32 Slot)
{
   /*---------------------
    * Local variables
    */
    epicsUInt32   offsetCR;                         /* Base address of offset to User-CSR space   */
    epicsUInt32   userCSRAdr = 0;                   /* Offset to User-CSR space (integer format)  */
    epicsUInt32   userCSRStart [CR_BEG_UCSR_BYTES]; /* Offset to User-CSR space (VME-64X format)  */

   /*---------------------
    * Get the offset in CR/CSR space for this slot.
    */
    offsetCR = CSR_BASE (Slot);

   /*---------------------
    * Read in the value of the offset to the beginning of User CSR space
    * Abort on error.
    */
    if (OK != vmeCSRMemProbe((offsetCR + CR_BEG_UCSR), CSR_READ, CR_BEG_UCSR_BYTES, userCSRStart))
        return 0;

   /*---------------------
    * Convert the offset to User-CSR space to a long integer
    */
    if (OK != convOffsetToLong(userCSRStart, CR_BEG_UCSR_BYTES, &userCSRAdr))
        return 0;

   /*---------------------
    * If no offest was specified in CR space, use the default offset for MRF boards
    */
    if (!userCSRAdr)
        userCSRAdr = MRF_USER_CSR_DEFAULT;

   /*---------------------
    * Add the converted User-CSR offset to the start of CR/CSR space
    * to get the address of the start of User-CSR space.
    *
    * Make sure the returned address is longword aligned.
    */
    return ((offsetCR + userCSRAdr) & ~3);

}/*end getUserCSRAdr()*/

/**************************************************************************************************
|* probeCRSpace () -- Copy CR Space into Local Memory Buffer
|*-------------------------------------------------------------------------------------------------
|*
|* Probes the specified VME slot for the existence of a board with CR/CSR capability.
|* If one is found, the contents of its CR space is copied into a local memory buffer.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = probeCRSpace (slot, pCR);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      slot       = (epicsInt32)    VME slot number to probe.
|*
|*-------------------------------------------------------------------------------------------------
|* OUTUT PARAMETERS:
|*      pCR        = (epicsUInt32 *) Address of a local memory buffer to copy the contents
|*                                   of this slot's CR space into.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status     = (epicsStatus)   Returns OK if we succesfully copied the contents of the
|*                                   specified slot's CR space into the local memory buffer.
|*                                   Returns ERROR otherwise.
|*
\**************************************************************************************************/

static
epicsStatus probeCRSpace (epicsInt32 slot, epicsUInt32 *pCR)
{
   /*---------------------
    * Local variables
    */
    epicsUInt32   offsetCR;     /* Offset to the start of CR/CSR space for this slot              */

   /*---------------------
    * Compute the offset in CR/CSR address space for this slot number.
    */
    offsetCR = CSR_BASE(slot);

   /*---------------------
    * Read the contents of CR-Space for this slot.
    * Return error if this is not a CR/CSR capable board.
    */
    return vmeCSRMemProbe(offsetCR, CSR_READ, CR_BYTES, pCR);

}/*end probeCRSpace()*/

/**************************************************************************************************
|* probeUCSRSpace () -- Copy User-CSR Space into Local Memory Buffer
|*-------------------------------------------------------------------------------------------------
|*
|* Locates the start of User-CSR space, either from the offset stored in the Configuration ROM
|* area, or from the default offset for MRF VME cards.  The contents of User-CSR space is then
|* copied into a local memory buffer.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = probeUCSRSpace (Slot, pUCSR);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      Slot       = (epicsInt32)    VME slot number to probe.
|*
|*-------------------------------------------------------------------------------------------------
|* OUTUT PARAMETERS:
|*      pUCSR      = (epicsUInt32 *) Address of a local memory buffer to copy the contents
|*                                   of this slot's User-CSR space into.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status     = (epicsStatus)   Returns OK if we succesfully copied the contents of the
|*                                   specified slot's User-CSR space into the local memory buffer.
|*                                   Returns ERROR otherwise.
|*
\**************************************************************************************************/

static
epicsStatus probeUCSRSpace (epicsInt32 Slot, epicsUInt32 *pUCSR)
{
   /*---------------------
    * Local variables
    */
    epicsUInt32  userCSRAdr;            /* Address of start of the User-CSR area                 */

   /*---------------------
    * Get the address of the start of User-CSR space for this slot.
    * Abort if the slot does not contain a card that supports CR/CSR addressing.
    */
    if (!(userCSRAdr = getUserCSRAdr(Slot)))
        return ERROR;

   /*---------------------
    * Attempt to read the contents of User-CSR space into the callers buffer.
    * Return whether or not we were successfull.
    */
    return vmeCSRMemProbe (userCSRAdr, CSR_READ, UCSR_BYTES, pUCSR);

}/*end probeUCSRSpace()*/

/**************************************************************************************************/
/*                              EPICS IOC Shell Registery                                         */
/*                                                                                                */


/**************************************************************************************************/
/*   mrfSetIrqLevel() -- Set the VME IRQ Level for the Board                                      */
/**************************************************************************************************/

static const iocshArg         mrfSetIrqLevelArg0    = {"slot",  iocshArgInt};
static const iocshArg         mrfSetIrqLevelArg1    = {"level", iocshArgInt};
static const iocshArg *const  mrfSetIrqLevelArgs[2] = {&mrfSetIrqLevelArg0, &mrfSetIrqLevelArg1};
static const iocshFuncDef     mrfSetIrqLevelDef     = {"mrfSetIrqLevel", 2, mrfSetIrqLevelArgs};

static void mrfSetIrqLevelCall (const iocshArgBuf *args) {
    mrfSetIrqLevel(args[0].ival, args[1].ival);
}/*end mrfSetIrqLevelCall()*/


/**************************************************************************************************/
/*   mrfSNShow() -- Print Board Serial Number                                                     */
/**************************************************************************************************/

static const iocshArg         mrfSNShowArg0    = {"slot", iocshArgInt};
static const iocshArg *const  mrfSNShowArgs[1] = {&mrfSNShowArg0};
static const iocshFuncDef     mrfSNShowDef     = {"mrfSNShow", 1, mrfSNShowArgs};
static void mrfSNShowCall (const iocshArgBuf *args) {
    mrfSNShow (args[0].ival);
}/*end mrfSNShowCall()*/


/**************************************************************************************************/
/*   vmeCRShow() -- Print Contents of CR space                                                    */
/**************************************************************************************************/

static const iocshArg         vmeCRShowArg0    = {"slot", iocshArgInt};
static const iocshArg *const  vmeCRShowArgs[1] = {&vmeCRShowArg0};
static const iocshFuncDef     vmeCRShowDef     = {"vmeCRShow", 1, vmeCRShowArgs};

static void vmeCRShowCall (const iocshArgBuf *args) {
    vmeCRShow (args[0].ival);
}/*end vmeCRShowCall()*/


/**************************************************************************************************/
/*   vmeCSRShow() -- Print Contents of CSR Space                                                  */
/**************************************************************************************************/

static const iocshArg         vmeCSRShowArg0    = {"slot", iocshArgInt};
static const iocshArg *const  vmeCSRShowArgs[1] = {&vmeCSRShowArg0};
static const iocshFuncDef     vmeCSRShowDef     = {"vmeCSRShow", 1, vmeCSRShowArgs};

static void vmeCSRShowCall(const iocshArgBuf *args) {
    vmeCSRShow (args[0].ival);
}/*end vmeCSRShowCall()*/


/**************************************************************************************************/
/*   vmeUserCSRShow() -- Print Contents of User CSR Space                                         */
/**************************************************************************************************/

static const iocshArg         vmeUserCSRShowArg0    = {"slot", iocshArgInt};
static const iocshArg *const  vmeUserCSRShowArgs[1] = {&vmeUserCSRShowArg0};
static const iocshFuncDef     vmeUserCSRShowDef     = {"vmeUserCSRShow", 1, vmeUserCSRShowArgs};

static void vmeUserCSRShowCall (const iocshArgBuf *args) {
    vmeUserCSRShow (args[0].ival);
}/*end vmeUserCSRShowCall()*/


/**************************************************************************************************/
/*   vmeCSRWriteADER() -- Write the ADER Register for the Board                                   */
/**************************************************************************************************/

static const iocshArg         vmeCSRWriteADERArg0    = {"slot", iocshArgInt};
static const iocshArg         vmeCSRWriteADERArg1    = {"func", iocshArgInt};
static const iocshArg         vmeCSRWriteADERArg2    = {"ader", iocshArgInt};
static const iocshArg *const  vmeCSRWriteADERArgs[3] = {&vmeCSRWriteADERArg0,
                                                        &vmeCSRWriteADERArg1,
                                                        &vmeCSRWriteADERArg2};
static const iocshFuncDef     vmeCSRWriteADERDef     = {"vmeCSRWriteADER", 3, vmeCSRWriteADERArgs};

static void vmeCSRWriteADERCall(const iocshArgBuf * args) {
    vmeCSRWriteADER (args[0].ival, args[1].ival, (epicsUInt32)args[2].ival);
}/*end vmeCSRWriteADERCall()*/


/**************************************************************************************************/
/*   Registrar Function                                                                           */
/**************************************************************************************************/

static void mrfVmeRegistrar () {
    iocshRegister (&mrfSetIrqLevelDef  , mrfSetIrqLevelCall  );
    iocshRegister (&mrfSNShowDef       , mrfSNShowCall       );
    iocshRegister (&vmeCRShowDef       , vmeCRShowCall       );
    iocshRegister (&vmeUserCSRShowDef  , vmeUserCSRShowCall  );
    iocshRegister (&vmeCSRShowDef      , vmeCSRShowCall      );
    iocshRegister (&vmeCSRWriteADERDef , vmeCSRWriteADERCall );
}/*end mrfVmeRegistrar()*/

epicsExportRegistrar (mrfVmeRegistrar);
