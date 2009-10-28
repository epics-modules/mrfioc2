/***************************************************************************************************
|* osiSyncIO.h -- Operating System Independent Synchronous I/O Routine Defintions
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   E.Bjorklund (LANSCE)
|* Date:     11 December 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 31 Dec 2006  E.Bjorklund	Original
|* 21 Oct 2009  E.Bjorklund     Expanded to handle both big-endian and little-endian busses
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*   This file maps the operating system independent synchronous I/O routines to their
|*   corresponding vxWorks synchronous I/O routines.
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

#ifndef OSI_SYNC_OPS_H
#define OSI_SYNC_OPS_H

/**************************************************************************************************/
/*  Required Header Files                                                                         */
/**************************************************************************************************/

#include  <vxWorks.h>                           /* vxWorks common definitions                     */
#include  <sysLib.h>                            /* vxWorks System Library Definitions             */
#include  <epicsTypes.h>                        /* EPICS Common Type Definitions                  */

/**************************************************************************************************/
/*  Function Prototypes for Routines Not Defined in sysLib.h                                      */
/**************************************************************************************************/

epicsUInt16 sysIn16    (void*);                 /* Synchronous 16 bit read                        */
epicsUInt32 sysIn32    (void*);                 /* Synchronous 32 bit read                        */
void        sysOut16   (void*, epicsUInt16);    /* Synchronous 16 bit write                       */
void        sysOut32   (void*, epicsUInt32);    /* Synchronous 32 bit write                       */

/**************************************************************************************************/
/*  Map the OSI Synchronous I/O Routines to Their vxWorks Counterparts                            */
/**************************************************************************************************/

/*---------------------
 * Definitions for Accessing Big-Endian Busses
 */
#define osi_read_be8_sync(address)        sysInByte   ((epicsUInt32)(address))
#define osi_read_be16_sync(address)       be16_to_cpu (sysIn16 ((address)))
#define osi_read_be32_sync(address)       be32_to_cpu (sysIn32 ((address)))

#define osi_write_be8_sync(address,data)  sysOutByte  ((epicsUInt32)(address), (epicsUInt8)(data))
#define osi_write_be16_sync(address,data) sysOut16    ((address), be16_to_cpu((epicsUInt16)(data)))
#define osi_write_be32_sync(address,data) sysOut32    ((address), be32_to_cpu((epicsUInt32)(data)))

/*---------------------
 * Definitions for Accessing Little-Endian Busses
 */
#define osi_read_le8_sync(address)        sysInByte   ((epicsUInt32)(address))
#define osi_read_le16_sync(address)       le16_to_cpu (sysIn16 ((address)))
#define osi_read_le32_sync(address)       le32_to_cpu (sysIn32 ((address)))

#define osi_write_le8_sync(address,data)  sysOutByte  ((epicsUInt32)(address), (epicsUInt8)(data))
#define osi_write_le16_sync(address,data) sysOut16    ((address), le16_to_cpu((epicsUInt16)(data)))
#define osi_write_le32_sync(address,data) sysOut32    ((address), le32_to_cpu((epicsUInt32)(data)))

#endif
