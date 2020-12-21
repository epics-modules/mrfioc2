/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef BUFRXMGR_H_INC
#define BUFRXMGR_H_INC


#include <ellLib.h>
#include <callback.h>

#include "mrf/databuf.h"
#include "evrMrmAPI.h"

class EVRMRM_API bufRxManager : public dataBufRx
{
public:
    bufRxManager(const std::string&, unsigned int qdepth, unsigned int bsize=0);

    virtual ~bufRxManager();

    unsigned int bsize(){return m_bsize;};

    epicsUInt8* getFree(unsigned int*);

    void receive(epicsUInt8*,unsigned int);

    /**@brief Notification if Rx queue overflows
     *
     * If the queue overflows then the function is invoked with 'len' zero
     * and 'buf' NULL.
     * If a buffer with a length greater then the given maxlen is
     * received then the function is invoked with 'len' set to the
     * length of the discarded buffer and 'buf' NULL.
     */
    virtual void dataRxError(dataBufComplete, void*) OVERRIDE FINAL;

    /**@brief Register to receive data buffers
     *
     *@param id Receive buffers with this Protocol ID.
     *@param fptr[in] Function pointer invoken after Rx
     *@param arg[in] Arbitrary pointer passed to completion function
     */
    virtual void dataRxAddReceive(dataBufComplete fptr, void* arg=0) OVERRIDE FINAL;

    /**@brief Unregister
     */
    virtual void dataRxDeleteReceive(dataBufComplete fptr, void* arg=0) OVERRIDE FINAL;

private:
    epicsMutex guard;

    struct listener {
        ELLNODE node;

        dataBufComplete fn;
        void *fnarg;
    };
    ELLLIST dispatch;

    dataBufComplete onerror;
    void* onerror_arg;

protected:
    void haderror(epicsStatus e){onerror(onerror_arg,e,0,NULL);}

private:
    ELLLIST freebufs;
    ELLLIST usedbufs;

    callbackPvt received_cb;
    static void received(callbackPvt*);

    struct buffer {
        ELLNODE node;
        unsigned int used;
        epicsUInt8 data[1]; //!< Actual length is bsize
    };

    const unsigned int m_bsize;
};

#endif // BUFRXMGR_H_INC
