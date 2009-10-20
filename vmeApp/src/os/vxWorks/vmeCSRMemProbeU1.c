/***************************************************************************************************
|* vmeCSRMemProbeU1.c -- vxWorks-Specific Routine to Probe VME CR/CSR Space
|*                       No BSP Support for CR/CSR Addressing.  Universe II VME Bridge.
|*                       Supports mv2100, mv2400, mv5100, and mv5500 processors
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   Eric Bjorklund (LANSCE)
|*
|* Date:     26 May 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 26 May 2006  E.Bjorklund     Original.
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This module contains the vxWorks-specific code for reading and writing to VME CR/CSR address
|* space when the processor's VME bridge is the Tundra Universe II chip and the BSP is for one
|* of the processors listed above.  This version of the code should be used if the Board Support
|* Package (BSP) does not support CR/CSR addressing.
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
|* Copyright (c) 2006 The University of Chicago,
|* as Operator of Argonne National Laboratory.
|*
|* Copyright (c) 2006 The Regents of the University of California,
|* as Operator of Los Alamos National Laboratory.
|*
|* Copyright (c) 2006 The Board of Trustees of the Leland Stanford Junior
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

#include <epicsTypes.h>         /* EPICS architecture-independent type definitions                */
#include <errlog.h>             /* EPICS error logging routines                                   */

#include <vxWorks.h>            /* vxWorks common definitions                                     */
#include <intLib.h>             /* vxWorks system-independent interrupt subroutine library        */
#include <sysLib.h>             /* vxWorks system-dependent library routines                      */
#include <vxLib.h>              /* vxWorks miscellaneous support routines                         */
#include <vme.h>                /* vxWorks VME address mode definitions                           */

#include <universe.h>           /* vxWorks VME Bridge chip definitions                            */
#include <mrfVme64x.h>          /* VME-64X CR/CSR definitions                                     */


/**************************************************************************************************/
/*  Constants and Macros                                                                          */
/**************************************************************************************************/

#define UNIVERSE_TARGET_WINDOW_COUNT  8

/*---------------------
 * Mask to locate an active target window mapped to A24 space
 */
#define LSI_CTL_WINDOW_MASK    (LSI_CTL_EN | LSI_CTL_USER2)

/*---------------------
 * Expected (masked) value of an active target window mapped to A24 space
 */
#define LSI_A24_WINDOW         (LSI_CTL_EN | LSI_CTL_A24)

/*---------------------
 * Value for an active target window mapped to CR/CSR space
 */
#define LSI_CSR_WINDOW (                                    \
    LSI_CTL_EN    |     /* Window is enabled              */\
    LSI_CTL_D64   |     /* 64-bit maximum data bus width  */\
    LSI_CTL_CSR)        /* CR/CSR Address Mode            */

/*---------------------
 * Table of Offsets to the 8 Target Window Control Registers
 */
LOCAL const epicsUInt32  lsiCtlX [UNIVERSE_TARGET_WINDOW_COUNT] = {
    0x100,        /* Control register for target window 0 */
    0x114,        /* Control register for target window 1 */
    0x128,        /* Control register for target window 2 */
    0x13c,        /* Control register for target window 3 */
    0x1a0,        /* Control register for target window 4 */
    0x1b4,        /* Control register for target window 5 */
    0x1c8,        /* Control register for target window 6 */
    0x1dc         /* Control register for target window 7 */
};

/*---------------------
 * Define the Universe Register Read Function
 */
#ifndef UNIV_IN_LONG
#define UNIV_IN_LONG(adr,pVal) *(epicsUInt32 *)pVal = sysPciInLong((epicsUInt32)(adr))
#endif /* UNIV_IN_LONG */

/*---------------------
 * Define the Universe Register Write Function
 */
#ifndef UNIV_OUT_LONG
#define UNIV_OUT_LONG(adr,val) sysPciOutLong((epicsUInt32)(adr),(epicsUInt32)(val))
#endif /* UNIV_OUT_LONG */

/**************************************************************************************************/
/*  External Functions and Data                                                                   */
/**************************************************************************************************/

IMPORT epicsUInt32 univBaseAdrs;        /* Base address for the Universe II register set          */

IMPORT epicsUInt32 sysPciInLong  (epicsUInt32 address);
IMPORT void        sysPciOutLong (epicsUInt32 address, epicsUInt32 data);

/**************************************************************************************************
|* vmeCSRMemProbe () -- Probe VME CR/CSR Address Space
|*-------------------------------------------------------------------------------------------------
|*
|* This routine probes the requested CR/CSR address space for read or write access and
|* either reads the number of bytes requested from CR/CSR space, or writes the number of
|* bytes requested to CR/CSR space.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    o Translate the CR/CSR address into the corresponding processor bus address for VME A24
|*      address space.
|*    o Locate the base address of the Universe chip's register set.
|*    o Locate the A24 target window.
|*    o Re-program the A24 window to look at CR/CSR space.
|*    o Probe the first byte of the CR/CSR address to make sure it is accessible.
|*    o If the first byte is accessible, transfer the rest of the data.
|*    o Restore the A24 window back to A24 address space.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = vmeCSRMemProbe (csrAddress, mode, length, &pVal);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      csrAddress  = (epicsUInt32)  Starting CR/CSR address to probe
|*      mode        = (epicsInt32)   CSR_READ or CSR_WRITE
|*      length      = (epicsInt32)   Number of bytes to read/write
|*
|*-------------------------------------------------------------------------------------------------
|* OUTUT PARAMETERS:
|*      pVal        = (epicsUInt32 *) Pointer to longword array which will contain either
|*                                    the data to be written or the data to be read.
|*                                    Actual data is read or written from the low-order byte
|*                                    of each element in the longword array.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status      = (epicsStatus)  Returns OK if the probe was successful.
|*                                   Returns an EPICS error code if the probe was not successful.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o In CR/CSR space, data is stored as bytes in longword arrays.  Only the third byte of each
|*   longword contains valid data.  Values that are larger than one byte must be assembled from
|*   the longword arrays. 
|*
|* o In order to be "endian-independent", this routine only reads the actual data bytes, which
|*   are found on third-byte boundaries (i.e. 0x3, 0x7, 0xB, 0xF).  Each data byte is returned
|*   in the low-order byte of a longword so that the data buffer will have the same longword
|*   indices as the values in CR/CSR space.
|*
\**************************************************************************************************/

GLOBAL_RTN
epicsStatus vmeCSRMemProbe (
    epicsUInt32   csrAddress,        /* Address in CR/CSR space to be probed.                     */
    epicsInt32    mode,              /* Probe direction (read or write)                           */
    epicsInt32    length,            /* Number of bytes to read/write                             */
    epicsUInt32  *pVal)              /* Data source (write) or sink (read)                        */
{
   /***********************************************************************************************/
   /*  Local Variables                                                                            */
   /***********************************************************************************************/

    epicsUInt8    firstByte;         /* First byte of the read or write operation                 */
    epicsInt32    i;                 /* Loop counter                                              */
    epicsUInt32   key;               /* Caller's interrupt level                                  */
    epicsUInt8   *localAddress;      /* Processor bus address corresponding to the CR/CSR address */
    epicsUInt32   lsiA24ctl;         /* Offset to the A24 target window control reg.              */
    epicsStatus   status;            /* Local status variable                                     */
    epicsUInt32   valA24ctl;         /* Old value of the A24 window's control register            */

   /***********************************************************************************************/
   /*  Code                                                                                       */
   /***********************************************************************************************/

   /*---------------------
    * Select the appropriate read or write code
    */
    if (mode == CSR_WRITE)
         mode = VX_WRITE;
    else mode = VX_READ;

   /*---------------------
    * Translate the CR/CSR address as if it were in A24 space
    */
    status = sysBusToLocalAdrs (
                 VME_AM_STD_SUP_DATA,
                 (char *)csrAddress,
                 (char **)(void *)&localAddress);

    if (status != OK) {
        errPrintf (
            -1, __FILE__, __LINE__,
            "  Unable to map CR/CSR address 0x%X inoto A24 space\n",
             csrAddress);
        return status;
    }/*end if could not translate the CR/CSR address*/

   /*---------------------
    * Make sure we have the base address for the Universe II register set.
    */
    if (univBaseAdrs == 0) {
        errPrintf (
            -1, __FILE__, __LINE__,
            "  Unable to get base address for Universe Chip.\n");
        return ERROR;
    }/*end if we could not find the base address for the Universe chip*/

   /*---------------------
    * Make sure the CR/CSR base address is aligned on a third-word boundary
    */
    localAddress = (epicsUInt8 *)((epicsUInt32)localAddress | 3);

   /*******************************************************************************************/
   /*                          -- Critical Section --                                         */
   /*        Lock out all interrupts while we manipulate the Universe registers               */
   /*                                                                                         */

    key = intLock();

   /*---------------------
    * Locate the VME target window that maps A24 space
    */
    for (i=0;  i < UNIVERSE_TARGET_WINDOW_COUNT;  i++) {
        lsiA24ctl = univBaseAdrs + lsiCtlX [i];
        UNIV_IN_LONG (lsiA24ctl, &valA24ctl);
        if (LSI_A24_WINDOW == (valA24ctl & LSI_CTL_WINDOW_MASK))
            break;
    }/*end loop to find A24 window*/

    if (i >= UNIVERSE_TARGET_WINDOW_COUNT) {
        intUnlock (key);
        errPrintf (-1, __FILE__, __LINE__,
                   "  Unable to locate an active A24 target window\n");
        return ERROR;
    }/*end could not locate active A24 window*/

   /*---------------------
    * Make the A24 target window map CR/CSR space instead
    */
    UNIV_OUT_LONG (lsiA24ctl, LSI_CSR_WINDOW);

   /*---------------------
    * Probe the first byte to make sure it can be read or written.
    * If the first byte was OK, transfer the rest of the data.
    */
    firstByte = pVal[0] & 0xff;
    status = vxMemProbe ((char *)localAddress, mode, 1, (char *)&firstByte);
    if (OK == status) {

        if (mode == CSR_WRITE) {
            for (i=1;  i < length;  i++) 
                localAddress[i*4] = pVal[i] & 0xff;
        }/*end if write access*/

        else { /* mode == CSR_READ */
            pVal[0] = firstByte;
            for (i=1;  i < length; i++)
                pVal[i] = localAddress[i*4];
        }/*end if read access*/

    }/*end if CR/CSR space is accessible*/

   /*---------------------
    * Restore the A24 window
    */
    UNIV_OUT_LONG (lsiA24ctl, valA24ctl);
    
   /*                                                                                         */
   /*                         -- End of Critical Section --                                   */
   /*******************************************************************************************/

   /*---------------------
    * Restore interrupt level and return
    */
    intUnlock (key);
    return status;

}/*end vmeCSRMemProbe()*/
