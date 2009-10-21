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
|*   corresponding RTEMS synchronous I/O routines.
|*
|*   Note that the RTEMS synchronous I/O routines also handle byte-swapping.
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

#include  <basicIoOps.h>                        /* RTEMS Synchronized I/O Operation Definitions   */
#include  <epicsTypes.h>                        /* EPICS Common Type Definitions                  */

/**************************************************************************************************/
/*  Map the OSI Synchronous I/O Routines to Their RTEMS Counterparts                              */
/**************************************************************************************************/

/*---------------------
 * Definitions for Accessing Big-Endian Busses
 */
#define osi_read_be8_sync(address)  in_8   ((volatile void *)(address))
#define osi_read_be16_sync(address) in_be16((volatile void *)(address))
#define osi_read_be32_sync(address) in_be32((volatile void *)(address))

#define osi_write_be8_sync(address,value)  \
        out_8   ((volatile void *)(address),  (epicsUInt8)(value))
#define osi_write_be16_sync(address,value) \
        out_be16((volatile void *)(address), (epicsUInt16)(value))
#define osi_write_be32_sync(address,value) \
        out_be32((volatile void *)(address), (epicsUInt32)(value))

/*---------------------
 * Definitions for Accessing Little-Endian Busses
 */
#define osi_read_le8_sync(address)  in_8   ((volatile void *)(address))
#define osi_read_le16_sync(address) in_le16((volatile void *)(address))
#define osi_read_le32_sync(address) in_le32((volatile void *)(address))

#define osi_write_le8_sync(address,value)  \
        out_8   ((volatile void *)(address),  (epicsUInt8)(value))
#define osi_write_le16_sync(address,value) \
        out_le16((volatile void *)(address), (epicsUInt16)(value))
#define osi_write_le32_sync(address,value) \
        out_le32((volatile void *)(address), (epicsUInt32)(value))

#endif
