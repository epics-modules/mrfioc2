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
 |* 22 Jan 2009  E.Bjorklund     Defined macros for use with all MRF hardware.
 |*
 |*--------------------------------------------------------------------------------------------------
 |* MODULE DESCRIPTION:
 |*
 |* This header file contains macro definitions for performing basic I/O operations on the hardware
 |* registers of the MRF Event System VME and PMC cards.
 |*
 |* The following operations are supported:
 |*   o 8-bit, 16-bit, and 32-bit scalar reads and writes.
 |*   o 8-bit, 16-bit, and 32-bit scalar bit set operations (read/modify/write).
 |*   o 8-bit, 16-bit, and 32-bit scalar bit clear operations (read/modify/write).
 |*   o 8-bit, 16-bit, and 32-bit scalar bit "flip" operations (read/modify/write),
 |*
 |* All I/O operations use underlying operating system routines to ensure that hardware pipelines
 |* are flushed so that the operations are executed in the order in which they were specified.
 |* The file "epicsMMIO.h", located in the OS-dependent directories (e.g., ./os/vxWorks/mrfIoOps.h,
 |* or ./os/RTEMS/epicsMMIO.h) maps synchronous I/O operations onto their OS-dependent routines or
 |* macros.
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
 |* The macros defined in this file can be classified within the following 6 catagories:
 |*
 |* Read Operations:
 |*     READ8  (base,offset)
 |*     READ16 (base,offset)
 |*     READ32 (base,offset)
 |*
 |* Write Operations:
 |*     WRITE8  (base,offset,value)
 |*     WRITE16 (base,offset,value)
 |*     WRITE32 (base,offset,value)
 |*
 |* Bit Set Operations:
 |*     BITSET8  (base,offset,mask)
 |*     BITSET16 (base,offset,mask)
 |*     BITSET32 (base,offset,mask)
 |*
 |* Bit Clear Operations:
 |*     BITCLR8  (base,offset,mask)
 |*     BITCLR16 (base,offset,mask)
 |*     BITCLR32 (base,offset,mask)
 |*
 |* Bit Flip Operations:
 |*     BITFLIP8  (base,offset,mask)
 |*     BITFLIP16 (base,offset,mask)
 |*     BITFLIP32 (base,offset,mask)
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

#ifndef MRF_COMMON_IO_H
#define MRF_COMMON_IO_H

/**************************************************************************************************/
/*  Include Other Header Files Needed by This Module                                              */
/**************************************************************************************************/

#include <epicsMMIO.h>           /* OS-dependent synchronous I/O routines                         */
#include <mrfBitOps.h>          /* Generic bit operations                                         */
#include <endian.h>

/**************************************************************************************************/
/*                            Macros For Accessing MRF Timing Modules                             */
/*            (Note that MRF timing modules are always accessed using native mode I/O             */
/**************************************************************************************************/

/*---------------------
 * Synchronous Read Operations
 */
#define READ8(base,offset)  NAT_READ8(base,offset)
#define READ16(base,offset) NAT_READ16(base,offset)
#define READ32(base,offset) NAT_READ32(base,offset)

/*---------------------
 * Synchronous Write Operations
 */
#define WRITE8(base,offset,value)  NAT_WRITE8(base,offset,value)
#define WRITE16(base,offset,value) NAT_WRITE16(base,offset,value)
#define WRITE32(base,offset,value) NAT_WRITE32(base,offset,value)

/*---------------------
 * Bit Set Operations
 */
#define BITSET8(base,offset,mask)   BITSET(NAT,8,base,offset,mask)
#define BITSET16(base,offset,mask)  BITSET(NAT,16,base,offset,mask)
#define BITSET32(base,offset,mask)  BITSET(NAT,32,base,offset,mask)

/*---------------------
 * Bit Clear Operations
 */
#define BITCLR8(base,offset,mask)   BITCLR(NAT,8,base,offset,mask)
#define BITCLR16(base,offset,mask)  BITCLR(NAT,16,base,offset,mask)
#define BITCLR32(base,offset,mask)  BITCLR(NAT,32,base,offset,mask)

/*---------------------
 * Bit Flip Operations
 */
#define BITFLIP8(base,offset,mask)   BITFLIP(NAT,8,base,offset,mask)
#define BITFLIP16(base,offset,mask)  BITFLIP(NAT,16,base,offset,mask)
#define BITFLIP32(base,offset,mask)  BITFLIP(NAT,32,base,offset,mask)

/**************************************************************************************************/
/*                                 Macros For Native Order I/O                                    */
/*                                                                                                */

/*================================================================================================*/
/* Define the macros for synchronous I/O operations.                                              */
/* These will ultimately resolve into operating system dependent function or macro calls.         */
/*================================================================================================*/

/**********
 * Little Endian address hack
 *
 * Using mrmEvg on cPCI on LE architecture leads to alot of problems since BE->LE conversion that is
 * done in hardware simply flips that lowes 2 address bits. This not a problem when word access is
 * used, but leads to incorrect memory access when using byte access (e.g. accessing 0x0 will actually
 * access 0x3, 0x1 will be 0x2 and so on)...
 *
 * This does not affect the behaviour of EVR since only word access is used...
 *
 * Change by: tslejko
 * Reason: cPCI EVG support
 */

/*---------------------
 * Synchronous Read Operations
 */
#if __BYTE_ORDER__ == __BIG_ENDIAN
#define NAT_READ8(base,offset)  \
        ioread8  ((epicsUInt8 *)(base) + U8_  ## offset)
#else
INLINE epicsUInt8 nat_read8_addrFlip(volatile void* addr) {
	switch ((size_t)addr % 4) {
	case 0:
<<<<<<< HEAD
		return ioread8((char*)addr + 3);
	case 1:
		return ioread8((char*)addr + 1);
	case 2:
		return ioread8((char*)addr - 1);
	case 3:
		return ioread8((char*)addr - 3);
=======
		return ioread8(((epicsUInt8 *)addr) + 3);
	case 1:
		return ioread8(((epicsUInt8 *)addr) + 1);
	case 2:
		return ioread8(((epicsUInt8 *)addr) - 1);
	case 3:
		return ioread8(((epicsUInt8 *)addr) - 3);
>>>>>>> 0cdcde317e77a73f3b7504288de308d19960578b
	}
}
#define NAT_READ8(base,offset)  \
		nat_read8_addrFlip  ((epicsUInt8 *)(base) + U8_  ## offset)
#endif

#if __BYTE_ORDER__ == __BIG_ENDIAN
#define NAT_READ16(base,offset) \
        nat_ioread16 ((epicsUInt8 *)(base) + U16_ ## offset)
#else
INLINE epicsUInt16 nat_ioread16_addrFlip(volatile void* addr){
  	switch ((size_t)addr % 4) {
  	case 0:
<<<<<<< HEAD
  		return nat_ioread16((char*)addr + 2);
  	case 2:
  		return nat_ioread16((char*)addr - 2);
=======
  		return nat_ioread16(((epicsUInt8 *)addr) + 2);
  	case 2:
  		return nat_ioread16(((epicsUInt8 *)addr) - 2);
>>>>>>> 0cdcde317e77a73f3b7504288de308d19960578b
  	}
  }
 #define NAT_READ16(base,offset) \
		 nat_ioread16_addrFlip ((epicsUInt8 *)(base) + U16_ ## offset)
#endif


#define NAT_READ32(base,offset) \
        nat_ioread32 ((epicsUInt8 *)(base) + U32_ ## offset)

/*---------------------
 * Synchronous Write Operations
 */
#if __BYTE_ORDER__ == __BIG_ENDIAN
#define NAT_WRITE8(base,offset,value) \
        iowrite8  (((epicsUInt8 *)(base) + U8_  ## offset),  value)
#else
INLINE void nat_write8_addrFlip(volatile void* addr, epicsUInt8 val){
 	switch ((size_t)addr % 4) {
 	case 0:
<<<<<<< HEAD
 		return iowrite8((char*)addr + 3,val);
 	case 1:
 		return iowrite8((char*)addr + 1,val);
 	case 2:
 		return iowrite8((char*)addr - 1,val);
 	case 3:
 		return iowrite8((char*)addr - 3,val);
=======
 		return iowrite8(((epicsUInt8 *)addr) + 3,val);
 	case 1:
 		return iowrite8(((epicsUInt8 *)addr) + 1,val);
 	case 2:
 		return iowrite8(((epicsUInt8 *)addr) - 1,val);
 	case 3:
 		return iowrite8(((epicsUInt8 *)addr) - 3,val);
>>>>>>> 0cdcde317e77a73f3b7504288de308d19960578b
 	}
 }
 #define NAT_WRITE8(base,offset,value) \
		nat_write8_addrFlip  (((epicsUInt8 *)(base) + U8_  ## offset),  value)
#endif

#if __BYTE_ORDER__ == __BIG_ENDIAN
#define NAT_WRITE16(base,offset,value) \
        nat_iowrite16 (((epicsUInt8 *)(base) + U16_ ## offset), value)
#else
INLINE void nat_iowrite16_addrFlip(volatile void* addr, epicsUInt16 val){
 	switch ((size_t)addr % 4) {
 	case 0:
<<<<<<< HEAD
 		return nat_iowrite16((short*)addr + 2,val);
 	case 2:
 		return nat_iowrite16((short*)addr - 2,val);
=======
 		return nat_iowrite16(((epicsUInt8 *)addr) + 2,val);
 	case 2:
 		return nat_iowrite16(((epicsUInt8 *)addr) - 2,val);
>>>>>>> 0cdcde317e77a73f3b7504288de308d19960578b
 	}
 }
#define NAT_WRITE16(base,offset,value) \
		nat_iowrite16_addrFlip (((epicsUInt8 *)(base) + U16_ ## offset), value)
#endif

#define NAT_WRITE32(base,offset,value) \
        nat_iowrite32 (((epicsUInt8 *)(base) + U32_ ## offset), value)

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
#define BE_READ8(base,offset)  \
        ioread8  ((epicsUInt8 *)(base) + U8_  ## offset)
#define BE_READ16(base,offset) \
        be_ioread16 ((epicsUInt8 *)(base) + U16_ ## offset)
#define BE_READ32(base,offset) \
        be_ioread32 ((epicsUInt8 *)(base) + U32_ ## offset)

/*---------------------
 * Synchronous Write Operations
 */
#define BE_WRITE8(base,offset,value) \
        iowrite8  (((epicsUInt8 *)(base) + U8_  ## offset),  value)
#define BE_WRITE16(base,offset,value) \
        be_iowrite16 (((epicsUInt8 *)(base) + U16_ ## offset), value)
#define BE_WRITE32(base,offset,value) \
        be_iowrite32 (((epicsUInt8 *)(base) + U32_ ## offset), value)


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
#define LE_READ8(base,offset)  \
        ioread8  ((epicsUInt8 *)(base) + U8_  ## offset)
#define LE_READ16(base,offset) \
        le_ioread16 ((epicsUInt8 *)(base) + U16_ ## offset)
#define LE_READ32(base,offset) \
        le_ioread32 ((epicsUInt8 *)(base) + U32_ ## offset)

/*---------------------
 * Synchronous Write Operations
 */
#define LE_WRITE8(base,offset,value) \
        iowrite8  (((epicsUInt8 *)(base) + U8_  ## offset),  value)
#define LE_WRITE16(base,offset,value) \
        le_iowrite16 (((epicsUInt8 *)(base) + U16_ ## offset), value)
#define LE_WRITE32(base,offset,value) \
        le_iowrite32 (((epicsUInt8 *)(base) + U32_ ## offset), value)

#endif
