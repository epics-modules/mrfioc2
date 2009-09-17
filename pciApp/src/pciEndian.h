
#ifndef PCIENDIAN_H_INC
#define PCIENDIAN_H_INC

#include <epicsEndian.h>

/* Host order <-> Little endian (PCI)
 */

#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
#  define htops(x) ( ((x<<8)&0xff00) | ((x>>8)&00ff) )

#  define htopl(x) ( ((x<<24)&0xff000000) | ((x<<8)&0x00ff0000) | \
                     ((x>>8)&0x0000ff00) | ((x>>24)&0x000000ff) )

#elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
#  define htops(x) ( x )
#  define htopl(x) ( x )
#else
#  error "RTEMS endianness macros undefined"
#endif

#endif /* PCIENDIAN_H_INC */
