
#ifndef MRFIOOPS_H
#define MRFIOOPS_H

#include <epicsEndian.h>

#if defined(_ARCH_PPC) || defined(__PPC__) || defined(__PPC)
#  include <libcpu/io.h>

/*NOTE: All READ/WRITE operations have an implicit read or write barrier */

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

/* Define native operations */
#  define nat_ioread16  be_ioread16
#  define nat_ioread32  be_ioread32
#  define nat_iowrite16 be_iowrite16
#  define nat_iowrite32 be_iowrite32

#elif defined(i386) ||defined(__i386__) || defined(__i386)

/* X86 does not need special handling for read/write width.
 *
 * TODO: Memory barriers?
 */

#include "mrfIoOpsDef.h"

#else
#  warning I/O operations not defined for this RTEMS architecture

#include "mrfIoOpsDef.h"

#endif /* if defined PPC */

#endif /* MRFIOOPS_H */
