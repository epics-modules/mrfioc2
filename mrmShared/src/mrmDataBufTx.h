/*************************************************************************\
* Copyright (c) 2014 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef MRMDATABUFTX_H_INC
#define MRMDATABUFTX_H_INC

#include <epicsMutex.h>
#include <mrmSharedAPI.h>

#include "mrf/databuf.h"

/**
 * With the MRM both the EVG and the EVR have
 * the exact same Tx control register
 */
class MRMSHARED_API mrmDataBufTx : public dataBufTx
{
public:

    mrmDataBufTx(const std::string& n,
                 volatile epicsUInt8* bufcontrol,
                 volatile epicsUInt8* buffer);
    virtual ~mrmDataBufTx();

    /* locking done internally */
    virtual void lock() const OVERRIDE FINAL {};
    virtual void unlock() const OVERRIDE FINAL {};

    virtual bool dataTxEnabled() const OVERRIDE FINAL;
    virtual void dataTxEnable(bool) OVERRIDE FINAL;

    virtual bool dataRTS() const OVERRIDE FINAL;

    virtual epicsUInt32 lenMax() const OVERRIDE FINAL;

    virtual void dataSend(epicsUInt32 len, const epicsUInt8 *buf) OVERRIDE FINAL;

private:
    volatile epicsUInt8 * const dataCtrl;
    volatile epicsUInt8 * const dataBuf;

    epicsMutex dataGuard;
};

#endif // MRMDATABUFTX_H_INC
