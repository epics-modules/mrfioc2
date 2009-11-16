
#ifndef MRFIOOPS_H
#define MRFIOOPS_H

#include <epicsEndian.h>

/*
 * Standard calls for operations for memory mapped I/O
 */

/********************** RTEMS **************************/
#if defined(__rtems__)

#if defined(_ARCH_PPC) || defined(__PPC__) || defined(__PPC)
#  include <libcpu/io.h>

/* All READ/WRITE operations have an implicit read or write barrier */

#  define READ8       in_8
#  define WRITE8     out_8
#  define LE_READ16   in_le16
#  define LE_READ32   in_le32
#  define LE_WRITE16 out_le16
#  define LE_WRITE32 out_le32
#  define BE_READ16   in_be16
#  define BE_READ32   in_be32
#  define BE_WRITE16 out_be16
#  define BE_WRITE32 out_be32

#  define RB()  iobarrier_r()
#  define WB()  iobarrier_w()
#  define RWB() iobarrier_rw()

/*#elif defined(__i386__) || defined(__i386)*/

#else
#  error I/O operations not defined for this archetecture

#endif /* if defined PPC */

/* Define native operations */
#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
#  define NAT_READ16  BE_READ16
#  define NAT_READ32  BE_READ32
#  define NAT_WRITE16 BE_WRITE16
#  define NAT_WRITE32 BE_WRITE32

#elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
#  define NAT_READ16  LE_READ16
#  define NAT_READ32  LE_READ32
#  define NAT_WRITE16 LE_WRITE16
#  define NAT_WRITE32 LE_WRITE32

#else
#  error Unable to determine native byte order
#endif

/* end if defined(__rtems__) */

/********************** VxWorks **************************/
#elif defined(__vxWorks__)

#include  <vxWorks.h>
#include  <sysLib.h>
#include  <epicsTypes.h>
/* TODO: what header for memory barriers? */

/* Function Prototypes for Routines Not Defined in sysLib.h */
epicsUInt16 sysIn16    (void*);
epicsUInt32 sysIn32    (void*);
void        sysOut16   (void*, epicsUInt16);
void        sysOut32   (void*, epicsUInt32);

#define READ8(address)           sysInByte   ((epicsUInt32)(address))
#define WRITE8(address,data)     sysOutByte  ((epicsUInt32)(address), (epicsUInt8)(data))

#define NAT_READ16(address)      sysIn16 ((address))
#define NAT_READ32(address)      sysIn32 ((address))

#define NAT_WRITE16(address,data) sysOut16(address,data)
#define NAT_WRITE32(address,data) sysOut32(address,data)

#define BE_READ16(address)       be16_to_cpu (sysIn16 ((address)))
#define BE_READ32(address)       be32_to_cpu (sysIn32 ((address)))

#define BE_WRITE16(address,data) sysOut16    ((address), be16_to_cpu((epicsUInt16)(data)))
#define BE_WRITE32(address,data) sysOut32    ((address), be32_to_cpu((epicsUInt32)(data)))

#define LE_READ16(address)       le16_to_cpu (sysIn16 ((address)))
#define LE_READ32(address)       le32_to_cpu (sysIn32 ((address)))

#define LE_WRITE16(address,data) sysOut16    ((address), le16_to_cpu((epicsUInt16)(data)))
#define LE_WRITE32(address,data) sysOut32    ((address), le32_to_cpu((epicsUInt32)(data)))

#  define RB()  VX_MEM_BARRIER_R()
#  define WB()  VX_MEM_BARRIER_W()
#  define RWB() VX_MEM_BARRIER_RW()

/* end if defined(__vxWorks__) */

#else

#error Direct I/O operations not defined for this OS

#endif /* if defined(OS) */

#endif /* MRFIOOPS_H */
