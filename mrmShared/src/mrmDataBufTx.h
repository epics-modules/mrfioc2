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

#include "mrf/databuf.h"

/**
 * With the MRM both the EVG and the EVR have
 * the exact same Tx control register
 */
class epicsShareClass mrmDataBufTx : public dataBufTx
{
public:

    mrmDataBufTx(const std::string& n,
                 volatile epicsUInt8* bufcontrol,
                 volatile epicsUInt8* buffer);
    virtual ~mrmDataBufTx();

    /* locking done internally */
    virtual void lock() const{};
    virtual void unlock() const{};

    virtual bool dataTxEnabled() const;
    virtual void dataTxEnable(bool);

    virtual bool dataRTS() const;

    virtual epicsUInt32 lenMax() const;

    virtual void dataSend(epicsUInt32 len, const epicsUInt8 *buf);

private:
    volatile epicsUInt8 * const dataCtrl;
    volatile epicsUInt8 * const dataBuf;

    epicsMutex dataGuard;
};

#endif // MRMDATABUFTX_H_INC
