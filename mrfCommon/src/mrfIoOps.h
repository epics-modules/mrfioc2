
#ifndef MRFIOOPS_H
#define MRFIOOPS_H

/*
 * Safe operations on I/O memory.
 *
 * This files defines a set of macros for access to Memory Mapped I/O
 *
 * They are named Tioread# and Tiowrite# where # can be 8, 16, or 32.
 * 'T' can either be 'le_', 'be_', or 'nat_' (except ioread8 and
 * iowrite8).
 *
 * The macros defined use OS specific extensions to ensure the following.
 *
 * 1) Width.  A 16 bit operation will not be broken into two 8 bit operations,
 *            or one half of a 32 bit operation.
 *
 * 2) Order.  Writes to two different registers will not be reordered.
 *            This only applies to MMIO operations, not between MMIO and
 *            normal memory operations.
 *
 * PCI access should use either 'le_' or 'be_' as determined by the
 * device byte order.
 *
 * VME access should always use 'nat_'.  If the device byte order is
 * little endian then an explicit swap is required.
 *
 * Examples:
 *
 * Big endian device:
 *
 * = PCI
 *
 * be_write16(base+off, 14);
 * var = be_read16(base+off);
 *
 * = VME
 *
 * nat_write16(base+off, 14);
 * var = nat_read16(base+off);
 *
 * Little endian device
 *
 * = PCI
 *
 * le_write16(base+off, 14);
 * var = le_read16(base+off);
 *
 * = VME
 *
 * nat_write16(base+off, bswap16(14));
 * var = bswap16(nat_write16(base+off));
 *
 * This difference arises because VME bridges impliment hardware byte
 * swapping on little endian systems, while PCI bridges do not.
 * Software accessing PCI devices must know if byte swapping is required.
 * This is conditial swap is implimented by the 'be_' and 'le_' macros.
 *
 * This is a fundamental difference between PCI and VME.
 *
 * Software accessing PCI _must_ do conditional swapping.
 *
 * Software accessing MVE must _not_ do conditional swapping.
 */

#include <epicsEndian.h>

#define bswap16(value) ((epicsUInt16) (  \
        (((epicsUInt16)(value) & 0x00ff) << 8)    |       \
        (((epicsUInt16)(value) & 0xff00) >> 8)))

#define bswap32(value) (  \
        (((epicsUInt32)(value) & 0x000000ff) << 24)   |                \
        (((epicsUInt32)(value) & 0x0000ff00) << 8)    |                \
        (((epicsUInt32)(value) & 0x00ff0000) >> 8)    |                \
        (((epicsUInt32)(value) & 0xff000000) >> 24))

/*
 * Standard calls for operations for memory mapped I/O
 */

/********************** RTEMS **************************/
#if defined(__rtems__)

#if defined(_ARCH_PPC) || defined(__PPC__) || defined(__PPC)
#  include <libcpu/io.h>

/* All READ/WRITE operations have an implicit read or write barrier */

#  define ioread8(A)         in_8((volatile epicsUInt8*)(A))
#  define iowrite8(A,D)      out_8((volatile epicsUInt8*)(A), D)
#  define le_ioread16(A)     in_le16((volatile epicsUInt16*)(A))
#  define le_ioread32(A)     in_le32((volatile epicsUInt32*)(A))
#  define le_iowrite16(A,D)  out_le16((volatile epicsUInt16*)(A), D)
#  define le_iowrite32(A,D)  out_le32((volatile epicsUInt32*)(A), D)
#  define be_ioread16(A)     in_be16((volatile epicsUInt16*)(A))
#  define be_ioread32(A)     in_be32((volatile epicsUInt32*)(A))
#  define be_iowrite16(A,D)  out_be16((volatile epicsUInt16*)(A), D)
#  define be_iowrite32(A,D)  out_be32((volatile epicsUInt32*)(A), D)

#  define rbarr()  iobarrier_r()
#  define wbarr()  iobarrier_w()
#  define rwbarr() iobarrier_rw()

/*#elif defined(__i386__) || defined(__i386)*/

#else
#  error I/O operations not defined for this RTEMS architecture

#endif /* if defined PPC */

/* Define native operations */
#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
#  define nat_ioread16  be_ioread16
#  define nat_ioread32  be_ioread32
#  define nat_iowrite16 be_iowrite16
#  define nat_iowrite32 be_iowrite32

#elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
#  define nat_ioread16  le_ioread16
#  define nat_ioread32  le_ioread32
#  define nat_iowrite16 le_iowrite16
#  define nat_iowrite32 le_iowrite32

#else
#  error Unable to determine native byte order
#endif

/* end if defined(__rtems__) */

/********************** VxWorks **************************/
#elif defined(__vxWorks__)

#include  <vxWorks.h>
#include  <sysLib.h>
#include  <sysALib.h> /*TODO: all BSPs? */
#include  <epicsTypes.h>
/* TODO: what header for memory barriers? */

#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
#  define be16_to_cpu(X)  (X)
#  define be32_to_cpu(X)  (X)
#  define le16_to_cpu(X) bswap16(X)
#  define le32_to_cpu(X) bswap32(X)

#elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
#  define be16_to_cpu(X)  bswap16(X)
#  define be32_to_cpu(X)  bswap32(X)
#  define le16_to_cpu(X)  (X)
#  define le32_to_cpu(X)  (X)

#else
#  error Unable to determine native byte order
#endif

#define ioread8(address)           sysInByte   ((epicsUInt32)(address))
#define iowrite8(address,data)     sysOutByte  ((epicsUInt32)(address), (epicsUInt8)(data))

#define nat_ioread16(address)      sysIn16 ((address))
#define nat_ioread32(address)      sysIn32 ((address))

#define nat_iowrite16(address,data) sysOut16(address,data)
#define nat_iowrite32(address,data) sysOut32(address,data)

#define be_ioread16(address)       be16_to_cpu (sysIn16 ((address)))
#define be_ioread32(address)       be32_to_cpu (sysIn32 ((address)))

#define be_iowrite16(address,data) sysOut16    ((address), be16_to_cpu((epicsUInt16)(data)))
#define be_iowrite32(address,data) sysOut32    ((address), be32_to_cpu((epicsUInt32)(data)))

#define le_ioread16(address)       le16_to_cpu (sysIn16 ((address)))
#define le_ioread32(address)       le32_to_cpu (sysIn32 ((address)))

#define le_iowrite16(address,data) sysOut16    ((address), le16_to_cpu((epicsUInt16)(data)))
#define le_iowrite32(address,data) sysOut32    ((address), le32_to_cpu((epicsUInt32)(data)))

#  define rbarr()  VX_MEM_BARRIER_R()
#  define wbarr()  VX_MEM_BARRIER_W()
#  define rwbarr() VX_MEM_BARRIER_RW()

/* end if defined(__vxWorks__) */

#else /* if defined(OS) */

/* So that code will compile on host systems define some (in)sane defaults
 */

#define ioread8(A) ( *(volatile epicsUInt8*)(A) )
#define nat_ioread16(A) ( *(volatile epicsUInt16*)(A) )
#define nat_ioread32(A) ( *(volatile epicsUInt32*)(A) )

#define iowrite8(A,D) do{ *(volatile epicsUInt8*)(A) = (D); }while(0)
#define nat_iowrite16(A,D) do{ *(volatile epicsUInt16*)(A) = (D); }while(0)
#define nat_iowrite32(A,D) do{ *(volatile epicsUInt32*)(A) = (D); }while(0)

#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
#  define be_ioread16(A)    nat_ioread16(A)
#  define be_ioread32(A)    nat_ioread32(A)
#  define be_iowrite16(A,D) nat_iowrite16(A,D)
#  define be_iowrite32(A,D) nat_iowrite32(A,D)

#  define le_ioread16(A)    bswap16(nat_ioread16(A))
#  define le_ioread32(A)    bswap32(nat_ioread32(A))
#  define le_iowrite16(A,D) nat_iowrite16(A,bswap16(D))
#  define le_iowrite32(A,D) nat_iowrite32(A,bswap32(D))

#elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
#  define be_ioread16(A)    bswap16(nat_ioread16(A))
#  define be_ioread32(A)    bswap32(nat_ioread32(A))
#  define be_iowrite16(A,D) nat_iowrite16(A,bswap16(D))
#  define be_iowrite32(A,D) nat_iowrite32(A,bswap32(D))

#  define le_ioread16(A)    nat_ioread16(A)
#  define le_ioread32(A)    nat_ioread32(A)
#  define le_iowrite16(A,D) nat_iowrite16(A,D)
#  define le_iowrite32(A,D) nat_iowrite32(A,D)

#else
#  error Unable to determine native byte order
#endif

#  define rbarr()  do{}while(0)
#  define wbarr()  do{}while(0)
#  define rwbarr() do{}while(0)

#endif /* if defined(OS) */

#endif /* MRFIOOPS_H */
