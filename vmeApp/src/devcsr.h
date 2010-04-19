
#ifndef DEVLIBCSR_H
#define DEVLIBCSR_H 1

/**@file devLibCSR.h
 *
 * This file contains macros for direct manipulation of the CSR/CR
 * registers.
 */

#include "devLib.h"
#include <epicsTypes.h>
#include "mrfIoOps.h"

#include "vmedefs.h"

#ifdef __cplusplus
extern "C" {

#  ifndef INLINE
#    define INLINE static inline
#  endif
#endif

struct VMECSRDevice {
	epicsUInt32 vendor,board,revision;
};

#define VMECSR_END {0,0,0}

#define VMECSRANY 0xFfffFfff

#define VMECSRSLOTMAX ((1<<5)-1)

/*
 * Returns the cpu address of the start of the CSR space
 * for slot N, or NULL if no card is present.
 */
epicsShareFunc
volatile unsigned char* devCSRProbeSlot(int slot);

/*
 * Acts like devCSRProbeSlot with the additional test of the VME64
 * ID fields.  Only slots with a matching card will return non-NULL.
 *
 * If info is non-NULL then the structure it points to will be filled
 * with the ID information of the matching card.
 */
epicsShareFunc
volatile unsigned char* devCSRTestSlot(
	const struct VMECSRDevice* devs,
	int slot,
	struct VMECSRDevice* info
);

/* The top 5 bits of the 24 bit CSR address are the slot number.
 * 
 * This macro gives the VME CSR base address for a slot.
 * Give this address to devBusToLocalAddr() with type atVMECSR
 */
#define CSRSlotBase(slot) ( (slot)<<19 )

#define CSRADER(addr,mod) ( ((addr)&0xFfffFf00) | ( ((mod)&0x3f)<<2 ) )

/* Read/Write CR for standard sizes
 */

#define CSRRead8(addr) ioread8(addr)

#define CSRRead16(addr) ( CSRRead8(addr)<<8 | CSRRead8(addr+4) )

#define CSRRead24(addr) ( CSRRead16(addr)<<8 | CSRRead8(addr+8) )

#define CSRRead32(addr) ( CSRRead24(addr)<<8 | CSRRead8(addr+12) )

#define CSRWrite8(addr,val) iowrite8(addr, val)

#define CSRWrite16(addr,val) \
do{ CSRWrite8(addr,(val&0xff00)>>8); CSRWrite8(addr+4,val&0xff); }while(0)

#define CSRWrite24(addr,val) \
do{ CSRWrite16(addr,(val&0xffff00)>>8); CSRWrite8(addr+8,val&0xff); }while(0)

#define CSRWrite32(addr,val) \
do{ CSRWrite24(addr,(val&0xffffff00)>>8); CSRWrite8(addr+12,val&0xff); }while(0)

/*
 * Utility functions
 */

/* Read CR for slot N and print to stdout.
 */
epicsShareExtern void vmecsrprint(int N,int verb);

/* Read CR from all slots and print to stdout.
 */
epicsShareExtern void vmecsrdump(int verb);

/* Common defininitions for registers found
 * in the Configuration Rom (CR) on VME64 and VME64x cards.
 *
 * These registers are addressed with the CSR
 * address space.
 *
 * The CR is a little strange in that all values are
 * single bytes (D8), but still have 4 byte spacing.
 * For example the Organizationaly Unique Identifier (OUI)
 * is 3 bytes long.  The first byte is offset 0x27, the
 * second is 0x2B, and the third is 0x2F.
 *
 * The following definitions were originally taken from the
 * mrfEventSystem IOC written by:
 *   Jukka Pietarinen (Micro-Research Finland, Oy)
 *   Till Straumann (SLAC)
 *   Eric Bjorklund (LANSCE)
 *
 * Corrected against 'The VMEBus Handbook' (Ch 5.6)
 * ISBN 1-885731-08-6
 */

/**************************************************************************************************/
/*  CR/CSR Configuration ROM (CR) Register Definitions                                            */
/**************************************************************************************************/

/* VME64 required CR registers */

#define  CR_ROM_CHECKSUM           0x0003 /* 8-bit checksum of Configuration ROM space            */
#define  CR_ROM_LENGTH             0x0007 /* Number of bytes in Configuration ROM to checksum     */
#define  CR_DATA_ACCESS_WIDTH      0x0013 /* Configuration ROM area (CR) data access method       */
#define  CSR_DATA_ACCESS_WIDTH     0x0017 /* Control/Status Reg area (CSR) data access method     */
#define  CR_SPACE_ID               0x001B /* CR/CSR space ID (VME64, VME64X, etc).                */

#define  CR_ASCII_C                0x001F /* ASCII "C" (identifies this as CR space)              */
#define  CR_ASCII_R                0x0023 /* ASCII "R" (identifies this as CR space)              */

#define  CR_IEEE_OUI               0x0027 /* IEEE Organizationally Unique Identifier (OUI)        */
#define  CR_IEEE_OUI_BYTES                     3   /* Number of bytes in manufacturer's OUI       */
#define  CR_BOARD_ID               0x0033 /* Manufacturer's board ID                              */
#define  CR_BOARD_ID_BYTES                     4   /* Number of bytes in manufacturer's OUI       */
#define  CR_REVISION_ID            0x0043 /* Manufacturer's board revision ID                     */
#define  CR_REVISION_ID_BYTES                  4   /* Number of bytes in board revision ID        */
#define  CR_ASCII_STRING           0x0053 /* Offset to ASCII string (manufacturer-specific)       */
#define  CR_PROGRAM_ID             0x007F /* Program ID code for CR space                         */

/* VME64x required CR registers */

#define  CR_BEG_UCR                0x0083 /* Offset to start of manufacturer-defined CR space     */
#define  CR_END_UCR                0x008F /* Offset to end of manufacturer-defined CR space       */
#define  CR_BEG_UCSR_BYTES                     3   /* Number of bytes in User CSR starting offset */

#define  CR_BEG_CRAM               0x009B /* Offset to start of Configuration RAM (CRAM) space    */
#define  CR_END_CRAM               0x00A7 /* Offset to end of Configuration RAM (CRAM) space      */

#define  CR_BEG_UCSR               0x00B3 /* Offset to start of manufacturer-defined CSR space    */
#define  CR_END_UCSR               0x00BF /* Offset to end of manufacturer-defined CSR space      */

#define  CR_BEG_SN                 0x00CB /* Offset to beginning of board serial number           */
#define  CR_END_SN                 0x00DF /* Offset to end of board serial number                 */

#define  CR_SLAVE_CHAR             0x00E3 /* Board's slave-mode characteristics                   */
#define  CR_UD_SLAVE_CHAR          0x00E7 /* Manufacturer-defined slave-mode characteristics      */

#define  CR_MASTER_CHAR            0x00EB /* Board's master-mode characteristics                  */
#define  CR_UD_MASTER_CHAR         0x00EF /* Manufacturer-defined master-mode characteristics     */

#define  CR_IRQ_HANDLER_CAP        0x00F3 /* Interrupt levels board can respond to (handle)       */
#define  CR_IRQ_CAP                0x00F7 /* Interrupt levels board can assert                    */

#define  CR_CRAM_WIDTH             0x00FF /* Configuration RAM (CRAM) data access method)         */

#define  CR_FN_DAWPR(N)  ( 0x0103 + (N)*0x04 ) /* N = 0 -> 7 */
                                          /* Start of Data Access Width Parameter (DAWPR) regs    */
#define  CR_DAWPR_BYTES                        1   /* Number of bytes in a DAWPR register         */

#define  CR_FN_AMCAP(N)  ( 0x0123 + (N)*0x20 ) /* N = 0 -> 7 */
                                          /* Start of Address Mode Capability (AMCAP) registers   */
#define  CR_AMCAP_BYTES                        8   /* Number of bytes in an AMCAP register        */

#define  CR_FN_XAMCAP(N) ( 0x0223 + (N)*0x80 ) /* N = 0 -> 7 */
                                          /* Start of Extended Address Mode Cap (XAMCAP) registers*/
#define  CR_XAMCAP_BYTES                      32   /* Number of bytes in an XAMCAP register       */

#define  CR_FN_ADEM(N)   ( 0x0623 + (N)*0x10 ) /* N = 0 -> 7 */
                                          /* Start of Address Decoder Mask (ADEM) registers       */
#define  CR_ADEM_BYTES                         4   /* Number of bytes in an ADEM register         */

#define  CR_MASTER_DAWPR           0x06AF /* Master Data Access Width Parameter                   */
#define  CR_MASTER_AMCAP           0x06B3 /* Master Address Mode Capabilities   (8 entries)       */
#define  CR_MASTER_XAMCAP          0x06D3 /* Master Extended Address Mode Capabilities (8 entries)*/

/*---------------------
 * Size (in total bytes) of CR space
 */
#define  CR_SIZE                          0x0750   /* Size of CR space (in total bytes)           */
#define  CR_BYTES                   (CR_SIZE>>2)   /* Number of bytes in CR space                 */

/**************************************************************************************************/
/*  CR/CSR Control and Status Register (CSR) Offsets                                              */
/**************************************************************************************************/

/* VME64 required CSR registers */

#define  CSR_BAR                0x7ffff /* Base Address Register (MSB of our CR/CSR address)      */
#define  CSR_BIT_SET            0x7fffb /* Bit Set Register (writing a 1 sets the control bit)    */
#define  CSR_BIT_CLEAR          0x7fff7 /* Bit Clear Register (writing a 1 clears the control bit)*/

/* VME64x required CSR registers */

#define  CSR_CRAM_OWNER         0x7fff3 /* Configuration RAM Owner Register (0 = not owned)       */
#define  CSR_UD_BIT_SET         0x7ffef /* User-Defined Bit Set Register (for user-defined fns)   */
#define  CSR_UD_BIT_CLEAR       0x7ffeb /* User-Defined Bit Clear Register (for user-defined fns) */
#define  CSR_FN_ADER(N) (0x7ff63 + (N)*0x10) /* N = 0 -> 7 */
                                        /* Function N Address Decoder Compare Register (1st byte) */
#define  CSR_ADER_BYTES                     4   /* Number of bytes in an ADER register            */

/*---------------------
 * Bit offset definitions for the Bit Set Status Register
 */
#define  CSR_BITSET_RESET_MODE           0x80   /* Module is in reset mode                        */
#define  CSR_BITSET_SYSFAIL_ENA          0x40   /* SYSFAIL driver is enabled                      */
#define  CSR_BITSET_MODULE_FAIL          0x20   /* Module has failed                              */
#define  CSR_BITSET_MODULE_ENA           0x10   /* Module is enabled                              */
#define  CSR_BITSET_BERR                 0x08   /* Module has asserted a Bus Error                */
#define  CSR_BITSET_CRAM_OWNED           0x04   /* CRAM is owned                                  */

/* Common things to set in CSR
 */

/* Set base address for VME64x function N */
INLINE
void
CSRSetBase(volatile void* base, epicsUInt8 N, epicsUInt32 addr, epicsUInt8 amod)
{
  volatile char* ptr=(volatile char*)base;
  if (N>7) return;
  CSRWrite32((ptr) + CSR_FN_ADER(N), CSRADER(addr,amod) );
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* DEVLIBCSR_H */
