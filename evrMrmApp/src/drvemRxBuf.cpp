/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */



#include <cstdio>
#include <errlog.h>

#ifdef _WIN32
	#include <Winsock2.h>
	#pragma comment (lib, "Ws2_32.lib")
#endif
#include <callback.h>
#include <mrfCommonIO.h>
#include <mrfBitOps.h>
#include <epicsInterrupt.h>

#define DATABUF_H_INC_LEVEL2
#include <epicsExport.h>
#include "evrRegMap.h"
#include "drvemRxBuf.h"

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
    return (READ32(base, DataBufCtrl) & DataBufCtrl_mode) != 0;
}

void
mrmBufRx::dataRxEnable(bool v)
{
    int key=epicsInterruptLock();
    if (v) {
        // start reception and switch to DBus+data
        BITSET(NAT,32,base, DataBufCtrl, DataBufCtrl_mode|DataBufCtrl_rx);
    } else {
        BITSET(NAT,32,base, DataBufCtrl, DataBufCtrl_stop); // stop reception
        BITCLR(NAT,32,base, DataBufCtrl, DataBufCtrl_mode); // switch to DBus only
    }
    epicsInterruptUnlock(key);
}

/* This callback is required from the ISR.
 * The RX interrupt is not diabled, but since reception is complete
 * the RX enable bit in the data buffer RX control register is cleared.
 * The no additional data will be received, or interrupt generated, until
 * we re-enable it.
 *
 * Further, since the DataBufCtrl and DataRx(i) registers are not used anywhere else
 * we can safely avoid locking while accessing them.
 */
extern int evrDebug;

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
            
            if(evrDebug>2)
            {
                printf("buffer %s: %02x %02x %02x %02x = %08x\n",
                    self.name().c_str(),
                    *(self.base+U32_DataRx(0)),
                    *(self.base+U32_DataRx(1)),
                    *(self.base+U32_DataRx(2)),
                    *(self.base+U32_DataRx(3)),
                    *(epicsUInt32*)(self.base+U32_DataRx(0)));
            }

            /* keep buffer in big endian mode (as sent by EVG) */
            for(unsigned int i=0; i<rsize; i+=4) {
                *(epicsUInt32*)(buf+i) = BE_READ32(self.base, DataRx(i));
            }
            self.receive(buf, rsize);
        }
    }

    WRITE32(self.base, DataBufCtrl, sts|DataBufCtrl_rx);
} catch(std::exception& e) {
    epicsPrintf("exception in mrmBufRx::drainbuf callback: %s\n", e.what());
}
}
