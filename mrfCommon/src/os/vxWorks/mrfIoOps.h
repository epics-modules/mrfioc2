#ifndef MRFIOOPS_H
#define MRFIOOPS_H

/**************************************************************************************************/
/*  Required Header Files                                                                         */
/**************************************************************************************************/

#include  <vxWorks.h>                           /* vxWorks common definitions                     */
#include  <sysLib.h>                            /* vxWorks System Library Definitions             */
#include  <epicsTypes.h>                        /* EPICS Common Type Definitions                  */
#include  <epicsEndian.h>                       /* EPICS Byte Order Definitions                   */

/**************************************************************************************************/
/*  Function Prototypes for Routines Not Defined in sysLib.h                                      */
/**************************************************************************************************/

epicsUInt16 sysIn16    (volatile void*);                 /* Synchronous 16 bit read               */
epicsUInt32 sysIn32    (volatile void*);                 /* Synchronous 32 bit read               */
void        sysOut16   (volatile void*, epicsUInt16);    /* Synchronous 16 bit write              */
void        sysOut32   (volatile void*, epicsUInt32);    /* Synchronous 32 bit write              */


#define bswap16(value) ((epicsUInt16) (  \
        (((epicsUInt16)(value) & 0x00ff) << 8)    |       \
        (((epicsUInt16)(value) & 0xff00) >> 8)))

#define bswap32(value) (  \
        (((epicsUInt32)(value) & 0x000000ff) << 24)   |                \
        (((epicsUInt32)(value) & 0x0000ff00) << 8)    |                \
        (((epicsUInt32)(value) & 0x00ff0000) >> 8)    |                \
        (((epicsUInt32)(value) & 0xff000000) >> 24))

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

#define rbarr()  VX_MEM_BARRIER_R()
#define wbarr()  VX_MEM_BARRIER_W()
#define rwbarr() VX_MEM_BARRIER_RW()

#endif /* MRFIOOPS_H */
