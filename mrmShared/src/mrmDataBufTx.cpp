
#include "mrmDataBufTx.h"

#include <cstdio>
#include <stdexcept>

#include <epicsThread.h>
#include <epicsInterrupt.h>

#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#ifndef bswap32
#define bswap32(value) (  \
        (((epicsUInt32)(value) & 0x000000ff) << 24)   |                \
        (((epicsUInt32)(value) & 0x0000ff00) << 8)    |                \
        (((epicsUInt32)(value) & 0x00ff0000) >> 8)    |                \
        (((epicsUInt32)(value) & 0xff000000) >> 24))
#endif

#define DataTxCtrl_done 0x100000
#define DataTxCtrl_run  0x080000
#define DataTxCtrl_trig 0x040000
#define DataTxCtrl_ena  0x020000
#define DataTxCtrl_mode 0x010000
#define DataTxCtrl_len_mask 0x0007fc

#define DataTxCtrl_len_max DataTxCtrl_len_mask

CardMap<dataBufTx> datatxmap;

dataBufTx::~dataBufTx() {}
dataBufRx::~dataBufRx() {}

mrmDataBufTx::mrmDataBufTx(const std::string& n,
                 volatile epicsUInt8* bufcontrol,
                 volatile epicsUInt8* buffer
) :dataBufTx(n)
  ,dataCtrl(bufcontrol)
  ,dataBuf(buffer)
  ,dataGuard()
{
}

mrmDataBufTx::~mrmDataBufTx()
{
}

bool
mrmDataBufTx::dataTxEnabled() const
{
    return nat_ioread32(dataCtrl) &
         (DataTxCtrl_ena|DataTxCtrl_mode);
}

void
mrmDataBufTx::dataTxEnable(bool v)
{
    dataGuard.lock();

    epicsUInt32 reg=nat_ioread32(dataCtrl);
    epicsUInt32 mask=DataTxCtrl_ena|DataTxCtrl_mode;
    if(v)
        reg |= mask;
    else
        reg &= ~mask;
    nat_iowrite32(dataCtrl, reg);

    dataGuard.unlock();
}

bool
mrmDataBufTx::dataRTS() const
{
    epicsUInt32 reg=nat_ioread32(dataCtrl);

    if (!(reg&(DataTxCtrl_ena|DataTxCtrl_mode)))
        throw std::runtime_error("Buffer Tx not enabled");
    if (reg&DataTxCtrl_done)
        return true;
    else if (reg&DataTxCtrl_run)
        return false;
    else
        throw std::runtime_error("Buffer Tx not running or done");
}


epicsUInt32
mrmDataBufTx::lenMax() const
{
    return DataTxCtrl_len_max-1;
}

void
mrmDataBufTx::dataSend(epicsUInt8 id,
                   epicsUInt32 len,
                   const epicsUInt8 *ubuf
)
{
    static const double quantum=epicsThreadSleepQuantum();

    if (len > DataTxCtrl_len_max-1)
        throw std::out_of_range("Tx buffer is too long");

    STATIC_ASSERT(DataTxCtrl_len_max%4==0);

    dataGuard.lock();

    // TODO: Timeout needed?
    while(!dataRTS()) epicsThreadSleep(quantum);

    // Zero length
    // Seems to be required?
    nat_iowrite32(dataCtrl, DataTxCtrl_ena|DataTxCtrl_mode);

    // start with proto id
    epicsUInt32 temp = id;
    len++;

    epicsUInt32 index;
    for(index=1; index<len; index++) {
        int byte=index%4;

        if(byte==0)
            temp=0;

        temp <<= 8;
        temp |= ubuf[index-1];

        if(byte==3)
            nat_iowrite32(&dataBuf[index-3], bswap32(temp) );
    }
	
    for(; index%4; index++) {
        temp <<= 8;
        len++;

        if(index%4==3)
            nat_iowrite32(&dataBuf[index-3], bswap32(temp) );
    }

    wbarr();

    epicsUInt32 reg=len&DataTxCtrl_len_mask;

    reg |= DataTxCtrl_trig|DataTxCtrl_ena|DataTxCtrl_mode;

    nat_iowrite32(dataCtrl, reg);

    while(!dataRTS()) epicsThreadSleep(quantum);

    dataGuard.unlock();
}
