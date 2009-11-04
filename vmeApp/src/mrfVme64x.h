/***************************************************************************************************
|* $(TIMING)/vmeApp/src/mrfVme64x.h -- Utilities to Support VME-64X CR/CSR Geographical Addressing
|*
|*--------------------------------------------------------------------------------------------------
|* Authors:  Jukka Pietarinen (Micro-Research Finland, Oy)
|*           Eric Bjorklund (LANSCE)
|* Date:     15 May 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 15 May 2006  E.Bjorklund     Original, Adapted from Jukka's vme64x_cr.h file
|* 12 Oct 2007  R.Hartmann      Updated to include the series 230 modules
|* 06 Jun 2008  E.Bjorklund     Moved Board ID codes to mrfCommon.h
|* 20 Oct 2009  E.Bjorklund     Adapted for the Modular Register Mask software
|* 29 Oct 2009  E.Bjorklund     Renamed mrfSetIrqLevel() to mrfSetIrq() and modified it to set
|*                              both the IRQ vector and level (needed by modular register map).
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*   This header file contains the board identification codes and manufacturer's
|*   IEEE ID code for each of the MRF timing modules.  It also contains function
|*   templates for the utility functions used to probe and manipulate the CR/CSR
|*   address space.
|*
|*--------------------------------------------------------------------------------------------------
|* HARDWARE SUPPORTED:
|*   Series 2xx Event Generator and Event Receiver Cards
|*   APS Register Mask
|*   Modular Register Mask
|*
|*--------------------------------------------------------------------------------------------------
|* OPERATING SYSTEMS SUPPORTED:
|*   vxWorks
|*   RTEMS
|*
\**************************************************************************************************/

/***************************************************************************************************
|*                           COPYRIGHT NOTIFICATION
|***************************************************************************************************
|*
|* THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
|* AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
|* AND IN ALL SOURCE LISTINGS OF THE CODE.
|*
|***************************************************************************************************
|*
|* This software is distributed under the EPICS Open License Agreement which
|* can be found in the file, LICENSE, included with this distribution.
|*
\**************************************************************************************************/

#ifndef MRF_VME_64X_H
#define MRF_VME_64X_H

/**************************************************************************************************/
/*  Other Header Files Required By This File                                                      */
/**************************************************************************************************/

#include <epicsTypes.h>                 /* EPICS Architecture-independent type definitions        */
#include <mrfCommon.h>                  /* MRF Common defintions                                  */


/**************************************************************************************************/
/*  Mode Definitions for the vmeCSRMemProbe Function                                              */
/**************************************************************************************************/

#define CSR_READ     0                  /* Read from CR/CSR Space                                 */
#define CSR_WRITE    1                  /* Write to CR/CSR Space                                  */

/**************************************************************************************************/
/*  Other Constants                                                                               */
/**************************************************************************************************/

#define MRF_MAX_VME_SLOT           21   /* Maximum VME slot number                                */

/**************************************************************************************************/
/*                             MRF Board Identification Codes                                     */
/*                                                                                                */

/**************************************************************************************************/
/*  Vendor ID Codes for Micro-Research Finland, Oy                                                */
/**************************************************************************************************/

#define MRF_VME_IEEE_OUI     0x000EB2   /* VME Organizationally Unique Identifier (OUI) for MRF   */
#define MRF_PCI_VENDOR_ID    0x1A3E     /* PCI Vendor ID for MRF                                  */

/**************************************************************************************************/
/*  Board ID Field Masks                                                                          */
/**************************************************************************************************/

#define MRF_BID_TYPE_MASK    0xFFFFFF00 /* Mask for Board ID Field                                */
#define MRF_BID_SERIES_MASK  0x000000FF /* Mask for Board Series Field                            */

/**************************************************************************************************/
/*  Generic VME Board Type Codes                                                                  */
/**************************************************************************************************/

#define MRF_VME_EVG_BID      0x45470000 /* VME Event Generator                                    */
#define MRF_VME_EVR_BID      0x45520000 /* VME Event Receiver                                     */
#define MRF_VME_EVR_RF_BID   0x45524600 /* VME Event Receiver with RF Recovery                    */

#define MRF_VME_GTX_BID      0x47545800 /* VME Electron Gun Trigger                               */
#define MRF_VME_4CT_BID      0x34435400 /* VME Four Channel Trigger                               */

/**************************************************************************************************/
/*  Series 200 Board ID Codes                                                                     */
/**************************************************************************************************/

#define MRF_VME_EVG200_BID   (MRF_VME_EVG_BID    | MRF_SERIES_200) /* VME Event Generator 200     */
#define MRF_VME_EVR200_BID   (MRF_VME_EVR_BID    | MRF_SERIES_200) /* VME Event Receiver 200      */
#define MRF_VME_EVR200RF_BID (MRF_VME_EVR_RF_BID | MRF_SERIES_200) /* VME EVR 200 w/ RF Recovery  */

#define MRF_VME_GTX200_BID   (MRF_VME_GTX_BID    | MRF_SERIES_200) /* VME Electron Gun Trigger    */
#define MRF_VME_4CT200_BID   (MRF_VME_4CT_BID    | MRF_SERIES_200) /* VME Four Channel Trigger    */


/**************************************************************************************************/
/*  Series 230 Board ID Codes                                                                     */
/**************************************************************************************************/

#define MRF_VME_EVG230_BID   (MRF_VME_EVG_BID    | MRF_SERIES_230) /* VME Event Generator 230     */
#define MRF_VME_EVR230_BID   (MRF_VME_EVR_BID    | MRF_SERIES_230) /* VME Event Receiver 230      */
#define MRF_VME_EVR230RF_BID (MRF_VME_EVR_RF_BID | MRF_SERIES_230) /* VME EVR 230 w/ RF Recovery  */



/**************************************************************************************************/
/*  Function Prototypes for CR/CSR Utility Routines                                               */
/**************************************************************************************************/

/*---------------------
 * Global routines called by driver and device support
 */
epicsInt32   mrfFindNextEVG        (epicsInt32);
epicsInt32   mrfFindNextEVG200     (epicsInt32);
epicsInt32   mrfFindNextEVG230     (epicsInt32);
epicsInt32   mrfFindNextEVR        (epicsInt32);
epicsInt32   mrfFindNextEVR200     (epicsInt32);
epicsInt32   mrfFindNextEVR230     (epicsInt32);
void         mrfGetSerialNumberVME (epicsInt32, char*);
epicsStatus  mrfSetAddress         (epicsInt32, epicsUInt32, epicsUInt32);
epicsStatus  mrfSetAddressEx       (epicsInt32, epicsUInt32, epicsUInt32, epicsUInt32);
epicsStatus  mrfSetIrq             (epicsInt32, epicsInt32, epicsInt32);

/*---------------------
 * Diagnostic routines that can be called from the IOC shell.
 */
void         mrfSNShow             (epicsInt32);
void         vmeCRShow             (epicsInt32);
void         vmeCSRShow            (epicsInt32);
void         vmeUserCSRShow        (epicsInt32);

/*---------------------
 * Global VME-64X CR/CSR utility routines
 */
epicsStatus  vmeCRFindBoard        (epicsInt32, epicsUInt32, epicsUInt32, epicsInt32*);
epicsStatus  vmeCRFindBoardMasked  (epicsInt32, epicsUInt32, epicsUInt32, epicsUInt32, epicsInt32*);
epicsStatus  vmeCRGetBID           (epicsInt32, epicsUInt32*);
epicsStatus  vmeCRGetMFG           (epicsInt32, epicsUInt32*);
epicsStatus  vmeCSRWriteADER       (epicsInt32, epicsUInt32, epicsUInt32);
epicsStatus  vmeCSRMemProbe        (epicsUInt32, epicsInt32, epicsInt32, epicsUInt32*);


#endif
