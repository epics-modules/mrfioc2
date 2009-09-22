
#ifndef PCIENDIAN_H_INC
#define PCIENDIAN_H_INC

#include <shareLib.h>
#include <epicsEndian.h>

/* Host order <-> Little endian (PCI)
 */

#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
static inline
epicsUInt16 htops(epicsUInt16 x)
{return ((x<<8)&0xff00) | ((x>>8)&0x00ff); };

static inline
epicsUInt32 htopl(epicsUInt32 x)
{return ((x<<24)&0xff000000) | ((x<<8)&0x00ff0000) | \
                     ((x>>8)&0x0000ff00) | ((x>>24)&0x000000ff) ;
};

#elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
#  define htops(x) ( x )
#  define htopl(x) ( x )
#else
#  error "EPICS endianness macros undefined"
#endif

/* On big and little endian the inverse operation is identical */
#  define ptohl htopl
#  define ptohs htops

static inline
epicsUInt32 read32le(volatile void* addr)
{ return htopl(*(volatile epicsUInt32*)addr); };

static inline
epicsUInt16 read16le(volatile void* addr)
{ return htops(*(volatile epicsUInt16*)addr); };

static inline
epicsUInt8 read8le(volatile void* addr)
{ return *(volatile epicsUInt8*)addr; };

static inline
void write32le(epicsUInt32 val, volatile void* addr)
{ *(volatile epicsUInt32*)addr = htopl(val); };

static inline
void write16le(epicsUInt16 val, volatile void* addr)
{ *(volatile epicsUInt16*)addr = htops(val); };

static inline
void write8le(epicsUInt8 val, volatile void* addr)
{ *(volatile epicsUInt8*)addr = val; };

#endif /* PCIENDIAN_H_INC */
