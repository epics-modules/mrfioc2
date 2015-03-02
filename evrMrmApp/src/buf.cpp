#include "buf.h"

#include <epicsExport.h>
#include "devObj.h"
#include "mrf/object.h"
#include "mrf/databuf.h"
#include "mrmDataBufTx.h"

#define BUF_TX      ":BUFTX"
#define BUF_RX      ":BUFRX"

struct buffer_info_t {
    mrmDataBufTx *bufTx;
    dataBufRx *bufRx;
};

extern "C" buffer_info *bufInit(char *dev_name) {

    buffer_info *data = NULL;
    mrf::Object *object = NULL;
    std::string bufTxName(dev_name);
    std::string bufRxName(dev_name);

    if(!(data = (buffer_info *)calloc(1, sizeof(buffer_info)))) {
        return NULL;
    }

    bufTxName += BUF_TX;
    bufRxName += BUF_RX;

    object = mrf::Object::getObject(bufTxName);
    if(!object) {
        errlogPrintf("bufInit: failed to find object '%s'\n", bufTxName.c_str());
    } else {
        data->bufTx = dynamic_cast<mrmDataBufTx*>(object);
    }

    object = mrf::Object::getObject(bufRxName);
    if(!object) {
        errlogPrintf("bufInit: failed to find object '%s'\n", bufRxName.c_str());
    } else {
        data->bufRx = dynamic_cast<dataBufRx*>(object);
    }

    if(!data->bufTx && !data->bufRx) {
        errlogPrintf("bufInit: ERROR: failed to find both objects!\n");
        free(data);
        data = NULL;
    }

    return data;
}

extern "C" buffer_status maxLen(buffer_info *data, epicsUInt32 *maxLength) {

    if(!data) {
        errlogPrintf("bufInit: ERROR: data not initialized!\n");
        return bufferDataNull;
    }

    if(!data->bufTx) {
        errlogPrintf("bufInit: ERROR: transfer structure not initialized!\n");
        return bufferTxNotInit;
    }

    *maxLength = data->bufTx->lenMax();

    return bufferOK;
}

extern "C" buffer_status bufSend(buffer_info *data, epicsUInt32 len, epicsUInt8 *buf) {

    if(!data) {
        errlogPrintf("bufInit: ERROR: data not initialized!\n");
        return bufferDataNull;
    }

    if(!data->bufTx) {
        errlogPrintf("bufInit: ERROR: transfer structure not initialized!\n");
        return bufferTxNotInit;
    }

    data->bufTx->dataSend(len, buf);

    return bufferOK;
}

extern "C" buffer_status bufRegCallback(buffer_info *data, void (*bufRecievedCallback)(void *, epicsStatus , epicsUInt32 , const epicsUInt8* ), void * pass) {

    if(!data) {
        errlogPrintf("bufInit: ERROR: data not initialized!\n");
        return bufferDataNull;
    }

    if(!data->bufRx) {
        errlogPrintf("bufInit: ERROR: receive structure not initialized!\n");
        return bufferRxNotInit;
    }

    data->bufRx->dataRxAddReceive(bufRecievedCallback, pass);

    return bufferOK;
}
