#include "buf.h"

#include <epicsExport.h>
#include "devObj.h"
#include "mrf/object.h"
#include "mrf/databuf.h"
#include "mrmDataBufTx.h"

#define BUF_RX      ":BUFRX"
#define BUF_TX      ":BUFTX"

struct bufferInfo {
    dataBufRx       *bufRx;
    mrmDataBufTx    *bufTx;
};

extern "C" bufferInfo_t *bufInit(char *dev_name) {

    bufferInfo_t *data  = NULL;
    mrf::Object *object = NULL;
    std::string bufRxName(dev_name);
    std::string bufTxName(dev_name);

    if(!(data = (bufferInfo_t *)calloc(1, sizeof(bufferInfo_t)))) {
        errlogPrintf("bufInit ERROR: failed to allocate memory for buffer info!\n");
        return NULL;
    }

    bufRxName += BUF_RX;
    bufTxName += BUF_TX;

    object = mrf::Object::getObject(bufRxName);
    if(!object) {
        errlogPrintf("bufInit WARNING: failed to find object '%s'\n", bufRxName.c_str());
    } else {
        data->bufRx = dynamic_cast<dataBufRx*>(object);
    }

    object = mrf::Object::getObject(bufTxName);
    if(!object) {
        errlogPrintf("bufInit WARNING: failed to find object '%s'\n", bufTxName.c_str());
    } else {
        data->bufTx = dynamic_cast<mrmDataBufTx*>(object);
    }

    if(!data->bufRx && !data->bufTx) {
        errlogPrintf("bufInit: ERROR: failed to find both objects!\n");
        free(data);
        data = NULL;
    }

    return data;
}

epicsStatus bufEnable(bufferInfo_t *data) {

    if(data && data->bufRx) {
        data->bufRx->dataRxEnable(true);
    }

    if(data && data->bufTx) {
        data->bufTx->dataTxEnable(true);
    }

    return statusOK;
}

epicsStatus bufDisable(bufferInfo_t *data) {

    if(data && data->bufRx) {
        data->bufRx->dataRxEnable(false);
    }

    if(data && data->bufTx) {
        data->bufTx->dataTxEnable(false);
    }

    return statusOK;
}

extern "C" epicsStatus bufMaxLen(bufferInfo *data, epicsUInt32 *maxLength) {

    if(!data->bufTx) {
        errlogPrintf("bufInit: ERROR: transfer structure not initialized!\n");
        return statusERROR;
    }

    try {
        *maxLength = data->bufTx->lenMax();
    } catch(std::exception &e) {
        errlogPrintf("Exception: %s\n", e.what());
        return statusERROR;
    }

    return statusOK;
}

extern "C" epicsStatus bufSend(bufferInfo *data, epicsUInt32 len, epicsUInt8 *buf) {

    if(!data->bufTx) {
        errlogPrintf("bufInit: ERROR: transfer structure not initialized!\n");
        return statusERROR;
    }

    try {
        data->bufTx->dataSend(len, buf);
    } catch(std::exception &e) {
        errlogPrintf("Exception: %s\n", e.what());
        return statusERROR;
    }

    return statusOK;
}

extern "C" epicsStatus bufRegCallback(bufferInfo *data, void (*bufRecievedCallback)(void *, epicsStatus , epicsUInt32 , const epicsUInt8* ), void * pass) {

    if(!data->bufRx) {
        errlogPrintf("bufInit: ERROR: receive structure not initialized!\n");
        return statusERROR;
    }

    try {
        data->bufRx->dataRxAddReceive(bufRecievedCallback, pass);
    } catch(std::exception &e) {
        errlogPrintf("Exception: %s\n", e.what());
        return statusERROR;
    }

    return statusOK;
}
