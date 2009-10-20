/***************************************************************************************************
|* vmeCSRMemProbeB.c -- RTEMS-Specific Routine to Probe VME CR/CSR Space
|*                      BSP Support for CR/CSR Addressing.  No EPICS Support.
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   Eric Bjorklund (LANSCE)
|*
|* Date:     24 May 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 16 May 2006  E.Bjorklund     Original.
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This module contains the RTEMS-specific code for reading and writing to VME CR/CSR address
|* space.  This particular version of the code assumes that the Board Support Package (BSP)
|* supports CR/CSR addressing.  If your BSP does not support CR/CSR addressint, you will need
|* to use one of the following modules:
|*     vmeCSRMemProbeU - If your VME bridge is the Tundra Universe II chip
|*     vmeCSRMemProbeT - If your VME bridge is the Tundra Tsi148 (Tempe) chip
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
#include <debugPrint.h>         /* Debug print routine                                            */

#include <rtems.h>              /* RTEMS standard header                                          */
#include <bsp/VME.h>            /* RTEMS VME address translation routines                         */
#include <bsp/bspExt.h>         /* RTEMS Memory probe routines                                    */

#include <mrfVme64x.h>          /* VME-64X CR/CSR definitions                                     */


/**************************************************************************************************/
/*  Constants and Macros                                                                          */
/**************************************************************************************************/

/*---------------------
 * Make sure that the VME CR/CSR address mode is defined
 */

#ifndef VME_AM_CSR
#define VME_AM_CSR  (0x2f)
#endif

/*---------------------
 * Debug Printing Level
 *  2 = dump errors;
 *  4 = dump information.
 */

int vme64_crFlag = 0;

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
|*    o Translate the CR/CSR address into the corresponding processor bus address.
|*    o Probe the first byte of the CR/CSR address to make sure it is accessible.
|*    o If the first byte is accessible, transfer the rest of the data.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = vmeCSRMemProbe (csrAddress, mode, length, &pVal);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      csrAddress  = (epicsUInt32)   Starting CR/CSR address to probe
|*      mode        = (epicsInt32)    CSR_READ or CSR_WRITE
|*      length      = (epicsInt32)    Number of bytes to read/write
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
|*      status      = (epicsStatus)   Returns OK if the probe was successful.
|*                                    Returns an EPICS error code if the probe was not successful.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* o This is the version of the code to us if your RTEMS BSP supports CR/CSR addressing but
|*   your EPICS version does not.
|*
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
    epicsUInt8   *localAddress;      /* Processor bus address corresponding to the CR/CSR address */
    epicsStatus   status;            /* Local status variable                                     */

   /***********************************************************************************************/
   /*  Code                                                                                       */
   /***********************************************************************************************/

    DEBUGPRINT (DP_INFO, vme64_crFlag,
               ("vmeCSRMemProbe: probing at vme addr: 0x%08x\n", csrAddress));  

   /*---------------------
    * Select the appropriate read or write code
    */
    if (mode == CSR_WRITE)
         mode = VX_WRITE;
    else mode = VX_READ;

   /*---------------------
    * Translate the CR/CSR address into its equivalent memory bus address
    */
    status = BSP_vme2local_adrs (VME_AM_CSR, (char *)csrAddress, (char **)(void *)&localAddress);
    if (OK != status) {
        DEBUGPRINT (DP_ERROR, vme64_crFlag,
                   ("vmeCSRMemProbe: BSP_vme2local_adrs returned %d\n", status));
        return status;
    }/*end if could not translate the CR/CSR address*/

   /*---------------------
    * Make sure the CR/CSR base address is aligned on a third-byte boundary
    */
    localAddress = (epicsUInt8 *)((epicsUInt32)localAddress | 3);
    DEBUGPRINT (DP_INFO, vme64_crFlag,
               ("vmeCSRMemProbe: BSP_vme2local_adrs got local addr=0x%08x\n",
               (epicsUInt32) localAddress));

   /*---------------------
    * Probe the first byte to make sure it can be read or written
    */
    firstByte = pVal[0] & 0xff;
    status = bspExtMemProbe ((char *)localAddress, mode, 1, (char *)&firstByte);
    if (OK != status)
        return status;

   /*---------------------
    * If the first byte was OK, transfer the rest of the data
    */
    if (mode == CSR_WRITE) {
        for (i=1;  i < length;  i++) 
            localAddress[i*4] = pVal[i] & 0xff;
    }/*end if write access*/

    else { /* mode == CSR_READ */
        pVal[0] = firstByte;
        for (i=1;  i < length; i++)
            pVal[i] = localAddress[i*4];
    }/*end if read access*/

   /*---------------------
    * If we made it this far, the probe succeeded.
    */
    return OK;

}/*end vmeCSRMemProbe()*/
