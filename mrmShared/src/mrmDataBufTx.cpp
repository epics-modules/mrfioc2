/*************************************************************************\
* Copyright (c) 2014 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#include <cstdio>
#include <stdexcept>

#include <epicsTypes.h>

#include <epicsThread.h>
#include <epicsInterrupt.h>

#include <mrfCommonIO.h>

#include "mrf/databuf.h"

#include <epicsExport.h>

#include "mrmDataBufTx.h"

#define DataTxCtrl_done 0x100000
#define DataTxCtrl_run  0x080000
#define DataTxCtrl_trig 0x040000
#define DataTxCtrl_ena  0x020000
#define DataTxCtrl_mode 0x010000
#define DataTxCtrl_len_mask 0x0007fc
#define DataTxCtrl_len_max  DataTxCtrl_len_mask

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
    return (nat_ioread32(dataCtrl) &
         (DataTxCtrl_ena|DataTxCtrl_mode)) != 0;
}

void
mrmDataBufTx::dataTxEnable(bool v)
{
    SCOPED_LOCK(dataGuard);

    epicsUInt32 reg=nat_ioread32(dataCtrl);
    epicsUInt32 mask=DataTxCtrl_ena|DataTxCtrl_mode;
    if(v)
        reg |= mask;
    else
        reg &= ~mask;
    nat_iowrite32(dataCtrl, reg);
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
    return DataTxCtrl_len_max;
}

void
mrmDataBufTx::dataSend(epicsUInt32 len,
                       const epicsUInt8 *ubuf
)
{
    STATIC_ASSERT(DataTxCtrl_len_max%4==0);

    if (len > DataTxCtrl_len_max)
        throw std::out_of_range("Tx buffer is too long");
    
    // len must be a multiple of 4
    len &= DataTxCtrl_len_mask;

    SCOPED_LOCK(dataGuard);

    // Zero length
    // Seems to be required?
    nat_iowrite32(dataCtrl, DataTxCtrl_ena|DataTxCtrl_mode);

    // Write 4 byte words over VME
    epicsUInt32 index;
    for(index=0; index<len; index+=4) {
        be_iowrite32(&dataBuf[index], *(epicsUInt32*)(&ubuf[index]) );
    }

    nat_iowrite32(dataCtrl, len|DataTxCtrl_trig|DataTxCtrl_ena|DataTxCtrl_mode);

    // Reading flushes output queue of VME bridge
    // Actual sending is so fast that we can use busy wait here
    // Measurements showed that we loop up to 17 times
    while(!(nat_ioread32(dataCtrl)&DataTxCtrl_done));
}
