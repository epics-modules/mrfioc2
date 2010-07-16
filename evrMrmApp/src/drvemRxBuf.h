#ifndef DRVEMRXBUF_H
#define DRVEMRXBUF_H

#include <callback.h>

#include "bufrxmgr.h"

class mrmBufRx : public bufRxManager
{
public:
    mrmBufRx(volatile void *base,unsigned int qdepth, unsigned int bsize=0);
    virtual ~mrmBufRx();

    virtual bool dataRxEnabled() const;
    virtual void dataRxEnable(bool);

    static void drainbuf(CALLBACK*);

protected:
    volatile unsigned char * const base;
};

#endif // DRVEMRXBUF_H
