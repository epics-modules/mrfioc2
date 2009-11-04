/***************************************************************************************************
|* mrfCommonIO.h -- Macros for Performing I/O Operations
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   Eric Bjorklund (LANSCE)
|* Date:     8 January 2007
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 08 Jan 2007  E.Bjorklund     Original
|* 21 Oct 2009  E.Bjorklund     Removed array macros.
|*                              Reorganized register "typing" mechanism.
|*                              Expanded to handle both big-endian and little-endian busses
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This header file contains macro definitions for performing basic I/O operations on the hardware
|* registers of the MRF Event System VME and PMC cards.  Both big-endian and little-endian busses
|* are supported:
|*
|* The following operations are supported:
|*   o 8-bit, 16-bit, and 32-bit scalar reads and writes.
|*   o 8-bit, 16-bit, and 32-bit register clear operations.
|*   o 8-bit, 16-bit, and 32-bit scalar bit set operations (read/modify/write).
|*   o 8-bit, 16-bit, and 32-bit scalar bit clear operations (read/modify/write).
|*
|* Synchronous versions of each of the above operations are also provided.  Synchronous I/O
|* operations rely on underlying operating system routines to ensure that hardware pipelines
|* are flushed so that the operations are executed in the order in which they were specified.
|* The file "osiSyncIO.h", located in the OS-dependent directories (e.g., ./os/vxWorks/osiSyncIO,
|* or ./os/RTEMS/osiSyncIO) maps synchronous I/O operations onto their OS-dependent routines or
|* macros.
|*
|* Usually, synchronous I/O is not required and using synchronous I/O operations can hurt
|* the performance of your processor. Synchronous I/O is required, however, in the following cases:
|*   o In an interrupt service routine (ISR), synchronous I/O is required to ensure that the
|*     interrupt is acknowledged (and the interrupt line de-asserted) before the ISR exits.
|*   o In a situation where a value is read from one register by first writing a value
|*     (such as an address or an index) to a different register, the write operation must be
|*     synchronous in order to ensure that it occurs before the "value register" is read.
|*
|* If desired, all I/O operations can be forced to be synchronous by specifying SYNC_IO=YES in
|* the MRF_CONFIG_SITE files, or by defining the symbol, MRF_SYNC_IO, before including this header
|* file for the first time.
|*
|* The I/O macros in this file are designed to be easily understood and to minimize errors.
|* Each macro begins with a "base" and an "offset" parameter.  The "base" parameter specifies
|* the address (in local space) of the card's register map. The "offset" parameter must be a
|* defined symbol of the form:
|*         "Uxx_OffsetName"
|* The "Uxx" part of the name specifies the size of the register in bits (U8, U16, or U32) and
|*  provides a form of "type declaration" for the symbol.
|*
|* Only the "OffsetName" part of the offset symbol should be specified when invoking one of the
|* I/O macros.  The macro itself will prepend the appropriate "Uxx_" string.  This will
|* automatically produce a compile-time error if you try to use the wrong sized macro for the
|* desired register (e.g., using an 8-bit read macro to access a 16-bit register).
|*
|*--------------------------------------------------------------------------------------------------
|* DEFINED MACROS:
|*
|* The macros defined in this file can be classified within the following 8 catagories:
|*                                              |
|* Big-Endian Read Operations:                  | Synchronous Big-Endian Read Operations:
|*     BE_READ8  (base,offset)                  |     BE_READ8_SYNC  (base,offset)
|*     BE_READ16 (base,offset)                  |     BE_READ16_SYNC (base,offset)
|*     BE_READ32 (base,offset)                  |     BE_READ32_SYNC (base,offset)
|*                                              |
|* Big-Endian Write Operations:                 | Synchronous Big-Endian Write Operations:
|*     BE_WRITE8  (base,offset,value)           |     BE_WRITE8_SYNC  (base,offset,value)
|*     BE_WRITE16 (base,offset,value)           |     BE_WRITE16_SYNC (base,offset,value)
|*     BE_WRITE32 (base,offset,value)           |     BE_WRITE32_SYNC (base,offset,value)
|*                                              |
|*     BE_CLEAR8  (base,offset)                 |     BE_CLEAR8_SYNC  (base,offset)
|*     BE_CLEAR16 (base,offset)                 |     BE_CLEAR16_SYNC (base,offset)
|*     BE_CLEAR32 (base,offset)                 |     BE_CLEAR32_SYNC (base,offset)
|*                                              |
|*     BE_BITCLR8  (base,offset,mask)           |     BE_BITCLR8_SYNC  (base,offset,mask)
|*     BE_BITCLR16 (base,offset,mask)           |     BE_BITCLR16_SYNC (base,offset,mask)
|*     BE_BITCLR32 (base,offset,mask)           |     BE_BITCLR32_SYNC (base,offset,mask)
|*                                              |
|*     BE_BITSET8  (base,offset,mask)           |     BE_BITSET8_SYNC  (base,offset,mask)
|*     BE_BITSET16 (base,offset,mask)           |     BE_BITSET16_SYNC (base,offset,mask)
|*     BE_BITSET32 (base,offset,mask)           |     BE_BITSET32_SYNC (base,offset,mask)
|*                                              |
|* Little-Endian Read Operations:               | Synchronous Little-Endian Read Operations:
|*     LE_READ8  (base,offset)                  |     LE_READ8_SYNC  (base,offset)
|*     LE_READ16 (base,offset)                  |     LE_READ16_SYNC (base,offset)
|*     LE_READ32 (base,offset)                  |     LE_READ32_SYNC (base,offset)
|*                                              |
|* Little-Endian Write Operations:              | Synchronous Little-Endian Write Operations:
|*     LE_WRITE8  (base,offset,value)           |     LE_WRITE8_SYNC  (base,offset,value)
|*     LE_WRITE16 (base,offset,value)           |     LE_WRITE16_SYNC (base,offset,value)
|*     LE_WRITE32 (base,offset,value)           |     LE_WRITE32_SYNC (base,offset,value)
|*                                              |
|*     LE_CLEAR8  (base,offset)                 |     LE_CLEAR8_SYNC  (base,offset)
|*     LE_CLEAR16 (base,offset)                 |     LE_CLEAR16_SYNC (base,offset)
|*     LE_CLEAR32 (base,offset)                 |     LE_CLEAR32_SYNC (base,offset)
|*                                              |
|*     LE_BITCLR8  (base,offset,mask)           |     LE_BITCLR8_SYNC  (base,offset,mask)
|*     LE_BITCLR16 (base,offset,mask)           |     LE_BITCLR16_SYNC (base,offset,mask)
|*     LE_BITCLR32 (base,offset,mask)           |     LE_BITCLR32_SYNC (base,offset,mask)
|*                                              |
|*     LE_BITSET8  (base,offset,mask)           |     LE_BITSET8_SYNC  (base,offset,mask)
|*     LE_BITSET16 (base,offset,mask)           |     LE_BITSET16_SYNC (base,offset,mask)
|*     LE_BITSET32 (base,offset,mask)           |     LE_BITSET32_SYNC (base,offset,mask)
|*                                              |
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

#ifndef MRF_COMMON_IO_H
#define MRF_COMMON_IO_H

/**************************************************************************************************/
/*  Include Other Header Files Needed by This Module                                              */
/**************************************************************************************************/

#include <osiSyncIO.h>          /* OS-Independent Synchronous I/O Routines                        */


/**************************************************************************************************/
/*  Define the "Byte Swap" Macros that translate between bus byte order and cpu byte order        */
/*    o be16_to_cpu(): 16-bit Big-Endian Bus to CPU Translation                                   */
/*    o be32_to_cpu(): 32-bit Big-Endian Bus to CPU Translation                                   */
/*    o le16_to_cpu(): 16-bit Little-Endian Bus to CPU Translation                                */
/*    o le32_to_cpu(): 32-bit Little-Endian Bus to CPU Translation                                */
/*                                                                                                */
/**************************************************************************************************/


/*=====================
 * CPU Byte Order is Little-Endian
 */

#if _BYTE_ORDER == _LITTLE_ENDIAN

#define be16_to_cpu(value) ((epicsUInt16) (  \
        (((value) & 0x00ff) << 8)    |       \
        (((value) & 0xff00) >> 8)))

#define be32_to_cpu (epicsUInt32 value) ((epicsUInt32) (  \
        (((value) & 0x000000ff) << 24)   |                \
        (((value) & 0x0000ff00) << 8)    |                \
        (((value) & 0x00ff0000) >> 8)    |                \
        (((value) & 0xff000000) >> 24)))

#define le16_to_cpu(value) (value)
#define le32_to_cpu(value) (value)

/*=====================
 * CPU Byte Order is Big-Endian
 */

#elif _BYTE_ORDER == _BIG_ENDIAN

#define be16_to_cpu(value) (value)
#define be32_to_cpu(value) (value)

#define le16_to_cpu(value) ((epicsUInt16) (  \
        (((value) & 0x00ff) << 8)    |       \
        (((value) & 0xff00) >> 8)))

#define le32_to_cpu (epicsUInt32 value) ((epicsUInt32) (  \
        (((value) & 0x000000ff) << 24)   |                \
        (((value) & 0x0000ff00) << 8)    |                \
        (((value) & 0x00ff0000) >> 8)    |                \
        (((value) & 0xff000000) >> 24)))

/*=====================
 * CPU Byte Order is Unknown
 */

#else
#  error "EPICS endianness macros undefined"
#endif

/**************************************************************************************************/
/*                             Macros For Big-Endian Bus I/O                                      */
/*                                                                                                */


/*================================================================================================*/
/* Define the macros for synchronous I/O operations.                                              */
/* These will ultimately resolve into operating system dependent function or macro calls.         */
/*================================================================================================*/

/*---------------------
 * Synchronous Read Operations
 */
#define BE_READ8_SYNC(base,offset)  \
        osi_read_be8_sync  ((epicsUInt8 *)(base) + U8_  ## offset)
#define BE_READ16_SYNC(base,offset) \
        osi_read_be16_sync ((epicsUInt8 *)(base) + U16_ ## offset)
#define BE_READ32_SYNC(base,offset) \
        osi_read_be32_sync ((epicsUInt8 *)(base) + U32_ ## offset)

/*---------------------
 * Synchronous Write Operations
 */
#define BE_WRITE8_SYNC(base,offset,value) \
        osi_write_be8_sync  (((epicsUInt8 *)(base) + U8_  ## offset),  value)
#define BE_WRITE16_SYNC(base,offset,value) \
        osi_write_be16_sync (((epicsUInt8 *)(base) + U16_ ## offset), value)
#define BE_WRITE32_SYNC(base,offset,value) \
        osi_write_be32_sync (((epicsUInt8 *)(base) + U32_ ## offset), value)

/*---------------------
 * Synchronous Register Clear Operations
 */
#define BE_CLEAR8_SYNC(base,offset)  BE_WRITE8_SYNC (base, offset, 0)
#define BE_CLEAR16_SYNC(base,offset) BE_WRITE16_SYNC(base, offset, 0)
#define BE_CLEAR32_SYNC(base,offset) BE_WRITE32_SYNC(base, offset, 0)

/*---------------------
 * Synchronous Bit Clear Operations
 */
#define BE_BITCLR8_SYNC(base,offset,mask) \
        BE_WRITE8_SYNC (base, offset, (BE_READ8_SYNC(base, offset)  & (epicsUInt8)~(mask)))
#define BE_BITCLR16_SYNC(base,offset,mask) \
        BE_WRITE16_SYNC(base, offset, (BE_READ16_SYNC(base, offset) & (epicsUInt16)~(mask)))
#define BE_BITCLR32_SYNC(base,offset,mask) \
        BE_WRITE32_SYNC(base, offset, (BE_READ32_SYNC(base, offset) & (epicsUInt32)~(mask)))

/*---------------------
 * Synchronous Bit Set Operations
 */
#define BE_BITSET8_SYNC(base,offset,mask) \
        BE_WRITE8_SYNC (base, offset, (BE_READ8_SYNC(base, offset)  | (epicsUInt8)(mask)))
#define BE_BITSET16_SYNC(base,offset,mask) \
        BE_WRITE16_SYNC(base, offset, (BE_READ16_SYNC(base, offset) | (epicsUInt16)(mask)))
#define BE_BITSET32_SYNC(base,offset,mask) \
        BE_WRITE32_SYNC(base, offset, (BE_READ32_SYNC(base, offset) | (epicsUInt32)(mask)))

/*================================================================================================*/
/* If SYNC_IO=YES is specified in the MRF_CONFIG_SITE files, or if MRF_SYNC_IO is defined         */
/* prior to including this header file for the first time, make all I/O synchronous.              */
/*================================================================================================*/

#ifdef MRF_SYNC_IO

/*---------------------
 * Make All Scalar Read Operations Synchronous
 */
#define BE_READ8(base,offset)  BE_READ8_SYNC (base,offset)
#define BE_READ16(base,offset) BE_READ16_SYNC(base,offset)
#define BE_READ32(base,offset) BE_READ32_SYNC(base,offset)

/*---------------------
 * Make All Scalar Write Operations Synchronous
 */
#define BE_WRITE8(base,offset,value)  BE_WRITE8_SYNC (base,offset,value)
#define BE_WRITE16(base,offset,value) BE_WRITE16_SYNC(base,offset,value)
#define BE_WRITE32(base,offset,value) BE_WRITE32_SYNC(base,offset,value)

#else

/*================================================================================================*/
/* Define macros for regular (non-synchronous) I/O operations                                     */
/*================================================================================================*/

/*---------------------
 * Regular Read Operations
 */
#define BE_READ8(base,offset)  \
        *((volatile epicsUInt8 *) ((epicsUInt8 *)(base) + U8_  ## offset))
#define BE_READ16(base,offset) \
        be16_to_cpu (*((volatile epicsUInt16 *)((epicsUInt8 *)(base) + U16_ ## offset)))
#define BE_READ32(base,offset) \
        be32_to_cpu (*((volatile epicsUInt32 *)((epicsUInt8 *)(base) + U32_ ## offset)))

/*---------------------
 * Regular Write Operations
 */
#define BE_WRITE8(base,offset,value) \
        *(volatile epicsUInt8 *)((epicsUInt8 *)(base)  + U8_  ## offset) = value
#define BE_WRITE16(base,offset,value) \
        *(volatile epicsUInt16 *)((epicsUInt8 *)(base) + U16_ ## offset) = be16_to_cpu (value)
#define BE_WRITE32(base,offset,value) \
        *(volatile epicsUInt32 *)((epicsUInt8 *)(base) + U32_ ## offset) = be32_to_cpu (value)

#endif

/*================================================================================================*/
/* Define macros for derived I/O operations.                                                      */
/* These will resolve into either regular or synchronous operations, depending on how             */
/* the SYNC_IO option was defined in the MRF_CONFIG_SITE files.                                   */
/*================================================================================================*/

/*---------------------
 * Register Clear Operations
 */
#define BE_CLEAR8(base,offset)  BE_WRITE8 (base, offset, 0)
#define BE_CLEAR16(base,offset) BE_WRITE16(base, offset, 0)
#define BE_CLEAR32(base,offset) BE_WRITE32(base, offset, 0)

/*---------------------
 * Bit Clear Operations
 */
#define BE_BITCLR8(base,offset,mask) \
        BE_WRITE8 (base, offset, (BE_READ8(base, offset)  & (epicsUInt8)~(mask)))
#define BE_BITCLR16(base,offset,mask) \
        BE_WRITE16(base, offset, (BE_READ16(base, offset) & (epicsUInt16)~(mask)))
#define BE_BITCLR32(base,offset,mask) \
        BE_WRITE32(base, offset, (BE_READ32(base, offset) & (epicsUInt32)~(mask)))

/*---------------------
 * Bit Set Operations
 */
#define BE_BITSET8(base,offset,mask) \
        BE_WRITE8 (base, offset, (BE_READ8(base, offset)  | (epicsUInt8)(mask)))
#define BE_BITSET16(base,offset,mask) \
        BE_WRITE16(base, offset, (BE_READ16(base, offset) | (epicsUInt16)(mask)))
#define BE_BITSET32(base,offset,mask) \
        BE_WRITE32(base, offset, (BE_READ32(base, offset) | (epicsUInt32)(mask)))

/**************************************************************************************************/
/*                            Macros For Little-Endian Bus I/O                                    */
/*                                                                                                */


/*================================================================================================*/
/* Define the macros for synchronous I/O operations.                                              */
/* These will ultimately resolve into operating system dependent function or macro calls.         */
/*================================================================================================*/

/*---------------------
 * Synchronous Read Operations
 */
#define LE_READ8_SYNC(base,offset)  \
        osi_read8_sync  ((epicsUInt8 *)(base) + U8_  ## offset)
#define LE_READ16_SYNC(base,offset) \
        osi_read_le16_sync ((epicsUInt8 *)(base) + U16_ ## offset)
#define LE_READ32_SYNC(base,offset) \
        osi_read_le32_sync ((epicsUInt8 *)(base) + U32_ ## offset)

/*---------------------
 * Synchronous Write Operations
 */
#define LE_WRITE8_SYNC(base,offset,value) \
        osi_write8_sync  (((epicsUInt8 *)(base) + U8_  ## offset),  value)
#define LE_WRITE16_SYNC(base,offset,value) \
        osi_write_le16_sync (((epicsUInt8 *)(base) + U16_ ## offset), value)
#define LE_WRITE32_SYNC(base,offset,value) \
        osi_write_le32_sync (((epicsUInt8 *)(base) + U32_ ## offset), value)

/*---------------------
 * Synchronous Register Clear Operations
 */
#define LE_CLEAR8_SYNC(base,offset)  LE_WRITE8_SYNC (base, offset, 0)
#define LE_CLEAR16_SYNC(base,offset) LE_WRITE16_SYNC(base, offset, 0)
#define LE_CLEAR32_SYNC(base,offset) LE_WRITE32_SYNC(base, offset, 0)

/*---------------------
 * Synchronous Bit Clear Operations
 */
#define LE_BITCLR8_SYNC(base,offset,mask) \
        LE_WRITE8_SYNC (base, offset, (LE_READ8_SYNC(base, offset)  & (epicsUInt8)~(mask)))
#define LE_BITCLR16_SYNC(base,offset,mask) \
        LE_WRITE16_SYNC(base, offset, (LE_READ16_SYNC(base, offset) & (epicsUInt16)~(mask)))
#define LE_BITCLR32_SYNC(base,offset,mask) \
        LE_WRITE32_SYNC(base, offset, (LE_READ32_SYNC(base, offset) & (epicsUInt32)~(mask)))

/*---------------------
 * Synchronous Bit Set Operations
 */
#define LE_BITSET8_SYNC(base,offset,mask) \
        LE_WRITE8_SYNC (base, offset, (LE_READ8_SYNC(base, offset)  | (epicsUInt8)(mask)))
#define LE_BITSET16_SYNC(base,offset,mask) \
        LE_WRITE16_SYNC(base, offset, (LE_READ16_SYNC(base, offset) | (epicsUInt16)(mask)))
#define LE_BITSET32_SYNC(base,offset,mask) \
        LE_WRITE32_SYNC(base, offset, (LE_READ32_SYNC(base, offset) | (epicsUInt32)(mask)))

/*================================================================================================*/
/* If SYNC_IO=YES is specified in the MRF_CONFIG_SITE files, or if MRF_SYNC_IO is defined         */
/* prior to including this header file for the first time, make all I/O synchronous.              */
/*================================================================================================*/

#ifdef MRF_SYNC_IO

/*---------------------
 * Make All Scalar Read Operations Synchronous
 */
#define LE_READ8(base,offset)  LE_READ8_SYNC (base,offset)
#define LE_READ16(base,offset) LE_READ16_SYNC(base,offset)
#define LE_READ32(base,offset) LE_READ32_SYNC(base,offset)

/*---------------------
 * Make All Scalar Write Operations Synchronous
 */
#define LE_WRITE8(base,offset,value)  LE_WRITE8_SYNC (base,offset,value)
#define LE_WRITE16(base,offset,value) LE_WRITE16_SYNC(base,offset,value)
#define LE_WRITE32(base,offset,value) LE_WRITE32_SYNC(base,offset,value)

#else

/*================================================================================================*/
/* Define macros for regular (non-synchronous) I/O operations                                     */
/*================================================================================================*/

/*---------------------
 * Regular Read Operations
 */
#define LE_READ8(base,offset)  \
        *((volatile epicsUInt8 *) ((epicsUInt8 *)(base) + U8_  ## offset))
#define LE_READ16(base,offset) \
        be16_to_cpu (*((volatile epicsUInt16 *)((epicsUInt8 *)(base) + U16_ ## offset)))
#define LE_READ32(base,offset) \
        be32_to_cpu (*((volatile epicsUInt32 *)((epicsUInt8 *)(base) + U32_ ## offset)))

/*---------------------
 * Regular Write Operations
 */
#define LE_WRITE8(base,offset,value) \
        *(volatile epicsUInt8 *)((epicsUInt8 *)(base)  + U8_  ## offset) = value
#define LE_WRITE16(base,offset,value) \
        *(volatile epicsUInt16 *)((epicsUInt8 *)(base) + U16_ ## offset) = be16_to_cpu (value)
#define LE_WRITE32(base,offset,value) \
        *(volatile epicsUInt32 *)((epicsUInt8 *)(base) + U32_ ## offset) = be32_to_cpu (value)

#endif

/*================================================================================================*/
/* Define macros for derived I/O operations.                                                      */
/* These will resolve into either regular or synchronous operations, depending on how             */
/* the SYNC_IO option was defined in the MRF_CONFIG_SITE files.                                   */
/*================================================================================================*/

/*---------------------
 * Register Clear Operations
 */
#define LE_CLEAR8(base,offset)  LE_WRITE8 (base, offset, 0)
#define LE_CLEAR16(base,offset) LE_WRITE16(base, offset, 0)
#define LE_CLEAR32(base,offset) LE_WRITE32(base, offset, 0)

/*---------------------
 * Bit Clear Operations
 */
#define LE_BITCLR8(base,offset,mask) \
        LE_WRITE8 (base, offset, (LE_READ8(base, offset)  & (epicsUInt8)~(mask)))
#define LE_BITCLR16(base,offset,mask) \
        LE_WRITE16(base, offset, (LE_READ16(base, offset) & (epicsUInt16)~(mask)))
#define LE_BITCLR32(base,offset,mask) \
        LE_WRITE32(base, offset, (LE_READ32(base, offset) & (epicsUInt32)~(mask)))

/*---------------------
 * Bit Set Operations
 */
#define LE_BITSET8(base,offset,mask) \
        LE_WRITE8 (base, offset, (LE_READ8(base, offset)  | (epicsUInt8)(mask)))
#define LE_BITSET16(base,offset,mask) \
        LE_WRITE16(base, offset, (LE_READ16(base, offset) | (epicsUInt16)(mask)))
#define LE_BITSET32(base,offset,mask) \
        LE_WRITE32(base, offset, (LE_READ32(base, offset) | (epicsUInt32)(mask)))

#endif
