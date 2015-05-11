#include "devObj.h"
#include "mrf/object.h"
#include "mrf/databuf.h"
#include "mrmDataBufTx.h"

#include <epicsExport.h>
#include "devMrmBuf.h"

#define BUF_RX      ":BUFRX"
#define BUF_TX      ":BUFTX"

struct mrmBufferInfo {
    dataBufRx       *bufRx;
    mrmDataBufTx    *bufTx;
};

extern "C" {

mrmBufferInfo_t *mrmBufInit(const char *dev_name) {

    mrmBufferInfo_t *data  = NULL;
    mrf::Object *object = NULL;
    std::string bufRxName(dev_name);
    std::string bufTxName(dev_name);

    if(!dev_name) {
        errlogPrintf("mrmBufInit ERROR: NULL device name!\n");
        return NULL;
    }

    if(!(data = (mrmBufferInfo_t *)calloc(1, sizeof(mrmBufferInfo_t)))) {
        errlogPrintf("mrmBufInit ERROR: failed to allocate memory for buffer info!\n");
        return NULL;
    }

    bufRxName += BUF_RX;
    bufTxName += BUF_TX;

    object = mrf::Object::getObject(bufRxName);
    if(!object) {
        errlogPrintf("mrmBufInit WARNING: failed to find object '%s'\n", bufRxName.c_str());
    } else {
        data->bufRx = dynamic_cast<dataBufRx*>(object);
    }

    object = mrf::Object::getObject(bufTxName);
    if(!object) {
        errlogPrintf("mrmBufInit WARNING: failed to find object '%s'\n", bufTxName.c_str());
    } else {
        data->bufTx = dynamic_cast<mrmDataBufTx*>(object);
    }

    if(!data->bufRx && !data->bufTx) {
        errlogPrintf("mrmBufInit: ERROR: failed to find both objects!\n");
        free(data);
        data = NULL;
    }

    return data;
}

epicsStatus mrmBufRxSupported(mrmBufferInfo_t *data) {

    if(!data) {
        errlogPrintf("mrmBufRxSupported ERROR: NULL data parameter!\n");
        return -1;
    }

    if(data->bufRx) {
        return 1;
    }

    return 0;
}

epicsStatus mrmBufTxSupported(mrmBufferInfo_t *data) {

    if(!data) {
        errlogPrintf("mrmBufTxSupported ERROR: NULL data parameter!\n");
        return -1;
    }

    if(data->bufTx) {
        return 1;
    }

    return 0;

}

epicsStatus mrmBufEnable(mrmBufferInfo_t *data) {

    if(data && data->bufRx) {
        data->bufRx->dataRxEnable(true);
    }

    if(data && data->bufTx) {
        data->bufTx->dataTxEnable(true);
    }

    return 0;
}

epicsStatus mrmBufDisable(mrmBufferInfo_t *data) {

    if(data && data->bufRx) {
        data->bufRx->dataRxEnable(false);
    }

    if(data && data->bufTx) {
        data->bufTx->dataTxEnable(false);
    }

    return 0;
}

epicsStatus mrmBufMaxLen(mrmBufferInfo *data, epicsUInt32 *maxLength) {

    if(!data->bufTx) {
        errlogPrintf("mrmBufMaxLen: ERROR: transfer structure not initialized!\n");
        return -1;
    }

    try {
        *maxLength = data->bufTx->lenMax();
    } catch(std::exception &e) {
        errlogPrintf("mrmBufMaxLen: EXCEPTION: %s\n", e.what());
        return -1;
    }

    return 0;
}

epicsStatus mrmBufSend(mrmBufferInfo *data, epicsUInt32 len, epicsUInt8 *buf) {

    if(!data->bufTx) {
        errlogPrintf("mrmBufSend: ERROR: transfer structure not initialized!\n");
        return -1;
    }

    try {
        data->bufTx->dataSend(len, buf);
    } catch(std::exception &e) {
        errlogPrintf("mrmBufSend: EXCEPTION: %s\n", e.what());
        return -1;
    }

    return 0;
}

epicsStatus mrmBufRegCallback(mrmBufferInfo *data, mrmBufRecievedCallback callback, void * pass) {

    if(!data->bufRx) {
        errlogPrintf("mrmBufRegCallback: ERROR: receive structure not initialized!\n");
        return -1;
    }

    try {
        data->bufRx->dataRxAddReceive(callback, pass);
    } catch(std::exception &e) {
        errlogPrintf("mrmBufRegCallback: EXCEPTION: %s\n", e.what());
        return -1;
    }

    return 0;
}

} /* extern "C" */
