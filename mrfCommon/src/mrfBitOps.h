/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef MRFBITOPS_H
#define MRFBITOPS_H

/*
 * I/O bit operations
 */

/*
 * ex. BITSET(BE,16,base,REGA,0x10)
 */
#define BITSET(ord,len,base,offset,mask) \
        ord ## _WRITE ## len(base,offset, (ord ## _READ ## len(base,offset) | (epicsUInt ## len)(mask)))
/*
 * ex. BITCLR(LE,32,base,REGB,0x80000)
 */
#define BITCLR(ord,len,base,offset,mask) \
        ord ## _WRITE ## len(base,offset, (ord ## _READ ## len(base,offset) & (epicsUInt ## len)~(mask)))

/*
 * ex. BITFLIP(BE,16,base,REGA,0x10)
 */
#define BITFLIP(ord,len,base,offset,mask) \
        ord ## _WRITE ## len(base,offset, (ord ## _READ ## len(base,offset) ^ (epicsUInt ## len)(mask)))

// Get U16 address
#define U16_REG(base, no) (base + (2 * (no)))

#endif /* MRFBITOPS_H */
