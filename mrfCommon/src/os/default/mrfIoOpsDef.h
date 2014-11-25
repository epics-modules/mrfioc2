
#ifndef MRFIOOPSDEF_H
#define MRFIOOPSDEF_H

#include <epicsTypes.h>
#include <epicsEndian.h>
#include <shareLib.h>

#ifdef __cplusplus
#  ifndef INLINE
#    define INLINE inline
#  endif
#endif

/* So that code will compile on host systems, define some (in)sane defaults
 */

INLINE
epicsUInt8
ioread8(volatile void* addr)
{
    return *(volatile epicsUInt8*)(addr);
}

INLINE
void
iowrite8(volatile void* addr, epicsUInt8 val)
{
    *(volatile epicsUInt8*)(addr) = val;
}

INLINE
epicsUInt16
nat_ioread16(volatile void* addr)
{
    return *(volatile epicsUInt16*)(addr);
}

INLINE
void
nat_iowrite16(volatile void* addr, epicsUInt16 val)
{
    *(volatile epicsUInt16*)(addr) = val;
}

INLINE
epicsUInt32
nat_ioread32(volatile void* addr)
{
    return *(volatile epicsUInt32*)(addr);
}

INLINE
void
nat_iowrite32(volatile void* addr, epicsUInt32 val)
{
    *(volatile epicsUInt32*)(addr) = val;
}

#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG

#define bswap16(value) ((epicsUInt16) (  \
        (((epicsUInt16)(value) & 0x00ff) << 8)    |       \
        (((epicsUInt16)(value) & 0xff00) >> 8)))

#define bswap32(value) (  \
        (((epicsUInt32)(value) & 0x000000ff) << 24)   |                \
        (((epicsUInt32)(value) & 0x0000ff00) << 8)    |                \
        (((epicsUInt32)(value) & 0x00ff0000) >> 8)    |                \
        (((epicsUInt32)(value) & 0xff000000) >> 24))

#  define be_ioread16(A)    nat_ioread16(A)
#  define be_ioread32(A)    nat_ioread32(A)
#  define be_iowrite16(A,D) nat_iowrite16(A,D)
#  define be_iowrite32(A,D) nat_iowrite32(A,D)

#  define le_ioread16(A)    bswap16(nat_ioread16(A))
#  define le_ioread32(A)    bswap32(nat_ioread32(A))
#  define le_iowrite16(A,D) nat_iowrite16(A,bswap16(D))
#  define le_iowrite32(A,D) nat_iowrite32(A,bswap32(D))

#elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE

#include <arpa/inet.h>

/* hton* is optimized or a builtin for most compilers
 * so use it if possible
 */
#define bswap16(v) htons(v)
#define bswap32(v) htonl(v)

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


#endif /* MRFIOOPSDEF_H */
