
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

#endif /* MRFBITOPS_H */
