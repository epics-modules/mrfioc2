#include "devObj.h"
#include "mrf/object.h"
#include "mrf/databuf.h"
#include "mrmDataBufTx.h"

#include <epicsExport.h>

#include "buf.h"

#define BUF_RX      ":BUFRX"
#define BUF_TX      ":BUFTX"

struct bufferInfo {
    dataBufRx       *bufRx;
    mrmDataBufTx    *bufTx;
};

extern "C" {

bufferInfo_t *bufInit(char *dev_name) {

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

    return 0;
}

epicsStatus bufDisable(bufferInfo_t *data) {

    if(data && data->bufRx) {
        data->bufRx->dataRxEnable(false);
    }

    if(data && data->bufTx) {
        data->bufTx->dataTxEnable(false);
    }

    return 0;
}

epicsStatus bufMaxLen(bufferInfo *data, epicsUInt32 *maxLength) {

    if(!data->bufTx) {
        errlogPrintf("bufMaxLen: ERROR: transfer structure not initialized!\n");
        return -1;
    }

    try {
        *maxLength = data->bufTx->lenMax();
    } catch(std::exception &e) {
        errlogPrintf("bufMaxLen: EXCEPTION: %s\n", e.what());
        return -1;
    }

    return 0;
}

epicsStatus bufSend(bufferInfo *data, epicsUInt32 len, epicsUInt8 *buf) {

    if(!data->bufTx) {
        errlogPrintf("bufSend: ERROR: transfer structure not initialized!\n");
        return -1;
    }

    try {
        data->bufTx->dataSend(len, buf);
    } catch(std::exception &e) {
        errlogPrintf("bufSend: EXCEPTION: %s\n", e.what());
        return -1;
    }

    return 0;
}

epicsStatus bufRegCallback(bufferInfo *data, bufRecievedCallback callback, void * pass) {

    if(!data->bufRx) {
        errlogPrintf("bufRegCallback: ERROR: receive structure not initialized!\n");
        return -1;
    }

    try {
        data->bufRx->dataRxAddReceive(callback, pass);
    } catch(std::exception &e) {
        errlogPrintf("bufRegCallback: EXCEPTION: %s\n", e.what());
        return -1;
    }

    return 0;
}

} /* extern "C" */
