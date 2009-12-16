
#ifndef MRFIOOPS_H
#define MRFIOOPS_H

#include <epicsEndian.h>

#define bswap16(value) ((epicsUInt16) (  \
        (((epicsUInt16)(value) & 0x00ff) << 8)    |       \
        (((epicsUInt16)(value) & 0xff00) >> 8)))

#define bswap32(value) (  \
        (((epicsUInt32)(value) & 0x000000ff) << 24)   |                \
        (((epicsUInt32)(value) & 0x0000ff00) << 8)    |                \
        (((epicsUInt32)(value) & 0x00ff0000) >> 8)    |                \
        (((epicsUInt32)(value) & 0xff000000) >> 24))

/* So that code will compile on host systems, define some (in)sane defaults
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

#define rbarr()  do{}while(0)
#define wbarr()  do{}while(0)
#define rwbarr() do{}while(0)


#endif /* MRFIOOPS_H */
