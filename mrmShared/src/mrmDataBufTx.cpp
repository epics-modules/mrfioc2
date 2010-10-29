
#include "mrmDataBufTx.h"

#include <cstdio>
#include <stdexcept>

#include <epicsThread.h>
#include <epicsInterrupt.h>

#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#define DataTxCtrl_len_max 0x7ff

#define DataTxCtrl_done 0x100000
#define DataTxCtrl_run  0x080000
#define DataTxCtrl_trig 0x040000
#define DataTxCtrl_ena  0x020000
#define DataTxCtrl_mode 0x010000
#define DataTxCtrl_len_mask 0x0007fc

CardMap<dataBufTx> datatxmap;

dataBufTx::~dataBufTx() {}
dataBufRx::~dataBufRx() {}

mrmDataBufTx::mrmDataBufTx(
                 volatile epicsUInt8* bufcontrol,
                 volatile epicsUInt8* buffer
) :dataBufTx()
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

    dataGuard.lock();

    // TODO: Timeout needed?
    while(!dataRTS()) epicsThreadSleep(quantum);

    // Zero length
    // Seems to be required?
    nat_iowrite32(dataCtrl, DataTxCtrl_ena|DataTxCtrl_mode);

    epicsUInt32 index = 0;
    iowrite8(&dataBuf[index], id);
    len++;

    for(index=1; index<len; index++)
        iowrite8(&dataBuf[index], ubuf[index-1]);
	
    if(len%4) {
        for(epicsUInt32 i = 4-len%4; i > 0; i--, index++) {
            len++;
            iowrite8(&dataBuf[index], 0);
        }
    }
    wbarr();

    epicsUInt32 reg=len&DataTxCtrl_len_mask;

    reg |= DataTxCtrl_trig|DataTxCtrl_ena|DataTxCtrl_mode;

    nat_iowrite32(dataCtrl, reg);

    while(!dataRTS()) epicsThreadSleep(quantum);

    dataGuard.unlock();
}
