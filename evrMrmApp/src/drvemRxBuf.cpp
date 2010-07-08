#include "drvemRxBuf.h"

#include <cstdio>
#include <errlog.h>
#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#include "evrRegMap.h"

#include "counters.h"

static Counter download_time   ("Rx download time"); // s per byte

mrmBufRx::mrmBufRx(volatile void *b,unsigned int qdepth, unsigned int bsize)
    :bufRxManager(qdepth, bsize)
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
    if (v)
        BITSET(NAT,32,base, DataBufCtrl, DataBufCtrl_mode|DataBufCtrl_rx);
    else
        BITSET(NAT,32,base, DataBufCtrl, DataBufCtrl_mode|DataBufCtrl_stop);
}

void
mrmBufRx::drainbuf(CALLBACK* cb)
{
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

            counter_start(download_time);

            unsigned int rsize=sts&DataBufCtrl_len_mask;

            if (rsize>bsize) {
                errlogPrintf("Received data buffer with size %u >= %u\n", rsize, bsize);
                rsize=bsize;
            }

            for(unsigned int i=0; i<rsize; i++)
                buf[i]=READ8(self.base, DataRx(i));
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
            counter_stop(download_time, rsize);
        }
    }

    WRITE32(self.base, DataBufCtrl, sts|DataBufCtrl_rx);
}
