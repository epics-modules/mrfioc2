/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifdef DATABUF_H_INC_LEVEL2
 #ifdef epicsExportSharedSymbols
  #define DATABUFL2_epicsExportSharedSymbols
  #undef epicsExportSharedSymbols
  #include "shareLib.h"
 #endif
#endif


#ifndef DATABUF_H_INC
#define DATABUF_H_INC

#include <epicsTypes.h>
#include <epicsTime.h>

#include "mrf/object.h"

/**
 *@param arg[in] Arbitrary pointer passed by user
 *@param ok Indicate if 'buf' holds valid data
 *@param len Number of bytes in buffer.
 *@param buf[in] Byte array
 */
typedef void (*dataBufComplete)(void *arg, epicsStatus ok,
                           epicsUInt32 len, const epicsUInt8* buf);


class epicsShareClass dataBufTx : public mrf::ObjectInst<dataBufTx> {
    struct impl;
    impl *pimpl;
public:
    explicit dataBufTx(const std::string& n) : mrf::ObjectInst<dataBufTx>(n) {}
    virtual ~dataBufTx()=0;

    //! Is card configured for buffer transmission?
    virtual bool dataTxEnabled() const=0;
    virtual void dataTxEnable(bool)=0;

    //! Is card ready to send a buffer?
    virtual bool dataRTS() const=0;

    virtual epicsUInt32 lenMax() const=0;

    /**@brief Transmit a byte array
     *
     *@param len Number of bytes to send
     *@param buf[in] Pointer to byte array to be sent
     */
    virtual void dataSend(epicsUInt32 len, const epicsUInt8 *buf)=0;

};



class epicsShareClass dataBufRx : public mrf::ObjectInst<dataBufRx> {
public:
    explicit dataBufRx(const std::string& n) : mrf::ObjectInst<dataBufRx>(n) {}

    virtual ~dataBufRx()=0;

    virtual bool dataRxEnabled() const=0;
    virtual void dataRxEnable(bool)=0;

    /**@brief Notification if Rx queue overflows
     *
     * If the queue overflows then the function is invoked with 'len' zero
     * and 'buf' NULL.
     * If a buffer with a length greater then the given maxlen is
     * received then the function is invoked with 'len' set to the
     * length of the discarded buffer and 'buf' NULL.
     */
    virtual void dataRxError(dataBufComplete, void*)=0;

    /**@brief Register to receive data buffers
     *
     *@param fptr[in] Function pointer invoken after Rx
     *@param arg[in] Arbitrary pointer passed to completion function
     */
    virtual void dataRxAddReceive(dataBufComplete fptr, void* arg=0)=0;

    /**@brief Unregister
     */
    virtual void dataRxDeleteReceive(dataBufComplete fptr, void* arg=0)=0;
};

#endif // DATABUF_H_INC

#ifdef DATABUFL2_epicsExportSharedSymbols
 #undef DATABUF_H_INC_LEVEL2
 #define epicsExportSharedSymbols
 #include "shareLib.h"
#endif
