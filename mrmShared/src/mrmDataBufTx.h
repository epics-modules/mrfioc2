#ifndef MRMDATABUFTX_H_INC
#define MRMDATABUFTX_H_INC

#include <epicsMutex.h>

#include "mrf/databuf.h"

/**
 * With the MRM both the EVG and the EVR have
 * the exact same Tx control register
 */
class mrmDataBufTx : public dataBufTx
{
public:

    mrmDataBufTx(const std::string& n,
                 volatile epicsUInt8* bufcontrol,
                 volatile epicsUInt8* buffer);
    virtual ~mrmDataBufTx();

    virtual bool dataTxEnabled() const;
    virtual void dataTxEnable(bool);

    virtual bool dataRTS() const;

    virtual epicsUInt32 lenMax() const;

    virtual void dataSend(epicsUInt8 id, epicsUInt32 len, const epicsUInt8 *buf);

private:
    volatile epicsUInt8 * const dataCtrl;
    volatile epicsUInt8 * const dataBuf;

    epicsMutex dataGuard;
};

#endif // MRMDATABUFTX_H_INC
