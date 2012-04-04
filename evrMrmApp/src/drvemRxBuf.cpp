/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include "drvemRxBuf.h"

#include <cstdio>
#include <errlog.h>
#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#include "evrRegMap.h"

#ifndef bswap32
#define bswap32(value) (  \
        (((epicsUInt32)(value) & 0x000000ff) << 24)   |                \
        (((epicsUInt32)(value) & 0x0000ff00) << 8)    |                \
        (((epicsUInt32)(value) & 0x00ff0000) >> 8)    |                \
        (((epicsUInt32)(value) & 0xff000000) >> 24))
#endif

mrmBufRx::mrmBufRx(const std::string& n, volatile void *b,unsigned int qdepth, unsigned int bsize)
    :bufRxManager(n, qdepth, bsize)
    ,base((volatile unsigned char *)b)
{
}

mrmBufRx::~mrmBufRx()
{
    BITSET(NAT,32,base, DataBufCtrl, DataBufCtrl_stop);
}

bool
mrmBufRx::dataRxEnabled() const
{
    return READ32(base, DataBufCtrl) & DataBufCtrl_mode;
}

void
mrmBufRx::dataRxEnable(bool v)
{
    if (v) {
        // start reception and switch to DBus+data
        BITSET(NAT,32,base, DataBufCtrl, DataBufCtrl_mode|DataBufCtrl_rx);
    } else {
        BITSET(NAT,32,base, DataBufCtrl, DataBufCtrl_stop); // stop reception
        BITCLR(NAT,32,base, DataBufCtrl, DataBufCtrl_mode); // switch to DBus only
    }
}

void
mrmBufRx::drainbuf(CALLBACK* cb)
{
try {
    void *vptr;
    callbackGetUser(vptr,cb);
    mrmBufRx& self=*static_cast<mrmBufRx*>(vptr);

    epicsUInt32 sts=READ32(self.base, DataBufCtrl);

    // Configured to send?
    if (!(sts&DataBufCtrl_mode))
        return;

    // Still receiving?
    if (sts&DataBufCtrl_rx)
        return;

    if (sts&DataBufCtrl_sumerr) {
        self.haderror(2);

    } else {
        unsigned int bsize;
        epicsUInt8 *buf=self.getFree(&bsize);

        if (!buf) {
            self.haderror(1);
        } else {

            unsigned int rsize=sts&DataBufCtrl_len_mask;

            if (rsize>bsize) {
                errlogPrintf("Received data buffer with size %u >= %u\n", rsize, bsize);
                rsize=bsize;
            }

            epicsUInt32 val=0;
            for(unsigned int i=0; i<rsize; i++) {
                if(i%4==0)
                    val=bswap32(READ32(self.base, DataRx(i)));
                buf[i] = val>>24;
                val<<=8;
            }

            if(rsize%4) {
                epicsUInt32 rem=bswap32(READ32(self.base, DataRx(rsize&~0x3)));
                // maximum of 3 iterations
                for(unsigned int i=rsize-rsize%4; i<rsize; i++) {
                    buf[i] = rem>>24; // take top byte
                    rem<<=8; // shift next byte up
                }
            }
/*
            const unsigned int dblen=10;
            char pbuf[dblen*2+1];
            for(unsigned int i=0; i<dblen; i++) {
                if(i<rsize) {
                    pbuf[2*i] = buf[i]/16<=9 ? '0' + buf[i]/16 : 'a' + buf[i]/16;
                    pbuf[2*i+1] = buf[i]%16<=9 ? '0' + buf[i]%16 : 'a' + buf[i]%16;
                } else {
                    pbuf[2*i] = '\0';
                    pbuf[2*i+1] = '\0';
                }
            }
            pbuf[dblen*2]='\0';
            errlogPrintf("Data %s\n",pbuf);
*/
            self.receive(buf, rsize);
        }
    }

    WRITE32(self.base, DataBufCtrl, sts|DataBufCtrl_rx);
} catch(std::exception& e) {
    epicsPrintf("exception in mrmBufRx::drainbuf callback: %s\n", e.what());
}
}
