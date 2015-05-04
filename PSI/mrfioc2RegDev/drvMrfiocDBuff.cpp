/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** drvMrfiocDBuff.cpp
**
** RegDev device support for Distributed Buffer on MRF EVR and EVG cards using
** mrfioc2 driver.
**
** -- DOCS ----------------------------------------------------------------
** Driver is registered via iocsh command:
**         mrfiocDBuffConfigure <regDevName> <mrfName> <protocol ID>
**
**             -regDevName: name of device as seen from regDev. E.g. this
**                         name must be the same as parameter 1 in record OUT/IN
**
**             -mrfName: name of mrf device
**
**             -protocol ID: protocol id (32 bit int). If set to 0, than receiver
**                         will accept all buffers. This is useful for
**                         debugging. If protocol != 0 then only received buffers
**                         with same protocol id are accepted. If you need to work
**                         with multiple protocols you can register multiple instances
**                         of regDev using the same mrfName but different regDevNames and
**                         protocols.
**
**
**         example:    mrfiocDBuffConfigure EVGDBUFF EVG1 42
**
**
** EPICS use:
**
**         - records behave the same as for any other regDev device with one exception:
**
**         - offset 0x00-0x04 can not be written to, since it is occupied by protocolID
**         - writing to offset 0x00 will flush the buffer
**         - input records are automatically processed (if scan == IO) when a valid buffer
**             is received
**
**
**
** -- SUPPORTED DEVICES -----------------------------------------------------
**
** VME EVG-230 (tx only, EVG does not support databuffer rx)
** VME EVR-230 (tx and rx)
** PCI EVR-230 (rx only, firmware version 3 does not support databuffer tx)
** PCIe EVR-300 (tx and rx)
**
**
** -- IMPLEMENTATION ---------------------------------------------------------
**
** In order to sync endianess across different devices and buses a following
** convention is followed.
**         - Data in distributed buffer is always BigEndian (4321). This also includes
**         data-types that are longer than 4bytes (e.g. doubles are 7654321)
**
**         - Data in scratch buffers (device->txBuffer, device->rxBuffer) is in
**         the same format as in hw buffer (always BigEndian).
**
**
** Device access routines mrfiocDBuff_flush, mrmEvrDataRxCB, implement
** correct conversions and data reconstructions. E.g. data received over PCI/
** PCIe will be in littleEndian, but the littleEndian conversion will be 4 bytes
** wide (this means that if data in HW is 76543210 the result will be 45670123)
**
**
** -- MISSING ---------------------------------------------------------------
**
** Explicit setting of DataBuffer MODE (whether DataBuffer is shared with DBUS)
**
**
** Author: Tom Slejko
** -------------------------------------------------------------------------*/

/*
 * mrfIoc2 headers
 */

#ifdef _WIN32
 #pragma comment (lib, "Ws2_32.lib")
 #include <Winsock2.h>
#else
#include <netinet/in.h> /* for htonl */ 
#endif
#include <stdlib.h>
#include <string.h>
/*
 *  EPICS headers
 */
#include <iocsh.h>
#include <drvSup.h>
#include <epicsExport.h>
#include <epicsEndian.h>
#include <regDev.h>
#include <devMrmBuf.h>
#include <errlog.h>

/*                                        */
/*        DEFINES                         */
/*                                        */

int drvMrfiocDBuffDebug = 0;
extern "C" {
 epicsExportAddress(int, drvMrfiocDBuffDebug);
}

#if defined __GNUC__ && __GNUC__ < 3
#define dbgPrintf(args...)  if(drvMrfiocDBuffDebug) printf(args);
#else
#define dbgPrintf(...)  if(drvMrfiocDBuffDebug) printf(__VA_ARGS__);
#endif


#define PROTO_LEN 4

/*
 * mrfDev reg driver private
 */
struct regDevice{
    char*                   name;            //regDevName of device
    mrmBufferInfo_t*        bufferHandle;         
    IOSCANPVT               ioscanpvt;
    epicsUInt8*             rxBuffer;        //pointer to 2k rx buffer
    epicsUInt8*             txBuffer;        //pointer to 2k tx buffer
    epicsUInt32             proto;           //protocol ID (4 bytes)
    epicsUInt16             usedTxBufferLen; //amount of data written in buffer (last touched byte)
};


/****************************************/
/*        DEVICE ACCESS FUNCIONS             */
/****************************************/

/*
 * Function will flush software scratch buffer.
 * Buffer will be copied and (if needed) its contents
 * will be converted into appropriate endiannes
 */
static void mrfiocDBuff_flush(regDevice* device)
{
    //Copy protocol ID
    *((epicsUInt32*) device->txBuffer) = htonl(device->proto);//Since everything in txBuffer is big endian

    mrmBufEnable(device->bufferHandle);

    if (drvMrfiocDBuffDebug)
    {
        printf("mrfiocDBuff_flush %s sending %u bytes\n", device->name, device->usedTxBufferLen);
        for (unsigned int i=0; i < device->usedTxBufferLen; i++)
        {
            printf(" %02x", device->txBuffer[i]);
        }
        printf("\n");
    }
    mrmBufSend(device->bufferHandle, device->usedTxBufferLen, device->txBuffer);

    // reset used length
    device->usedTxBufferLen = 0;
}


/*
 *    This callback is attached to EVR rx buffer logic.
 *    Byte order is big endian as on network.
 *    Callback then compares received protocol ID
 *    with desired protocol. If there is a match then buffer
 *    is copied into device private rxBuffer and scan IO
 *    is requested.
 */
void mrmEvrDataRxCB(void *pvt, epicsStatus ok,
            epicsUInt32 len, const epicsUInt8* buf)
{
    regDevice* device = (regDevice *)pvt;
    if (drvMrfiocDBuffDebug)
    {
        printf("mrmEvrDataRxCB %s: received %u bytes\n", device->name, len);
        for (unsigned int i=0; i < len; i++)
        {
            printf(" %02x", buf[i]);
        }
        printf("\n");
    }

    // Extract protocol ID (big endian)
    epicsUInt32 receivedProtocolID = ntohl(*(epicsUInt32*)(buf));

    dbgPrintf("mrmEvrDataRxCB %s: protocol ID = %d\n", device->name, receivedProtocolID);

    // Accept all protocols if devices was initialized with protocol == 0
    // or if the set protocol matches the received one
    if(!device->proto | (device->proto == receivedProtocolID));
    else return;

    dbgPrintf("Received new DATA!!! len: %d\n", len);
    memcpy(device->rxBuffer, buf, len);

    scanIoRequest(device->ioscanpvt);
}


/****************************************/
/*            REG DEV FUNCIONS              */
/****************************************/

void mrfiocDBuff_report(regDevice* device, int level)
{
    epicsUInt32 maxLength;

    mrmBufMaxLen(device->bufferHandle, &maxLength);
    printf("%s dataBuffer max length %u\n", device->name, maxLength);
}

/*
 * Read will make sure that the data is correctly copied into the records.
 * Since data in MRF DBUFF is big endian (since all EVGs are running on BE
 * systems) the data may need to be converted to LE. Data in rxBuffer is
 * always BE.
 */
int mrfiocDBuff_read(
        regDevice* device,
        size_t offset,
        unsigned int datalength,
        size_t nelem,
        void* pdata,
        int priority,
        regDevTransferComplete callback,
        char* user)
{
    dbgPrintf("mrfiocDBuff_read %s: from 0x%x len: 0x%x\n",
        device->name, (int)offset, (int)(datalength*nelem));

    if(offset < PROTO_LEN){
        errlogPrintf("mrfiocDBuff_read %s: device %s: byte offset must be greater than %d\n", 
                user, device->name, PROTO_LEN);
        return -1;
    }

    /* Data in buffer is in big endian byte order */
    regDevCopy(datalength, nelem, &device->rxBuffer[offset], pdata, NULL, REGDEV_SWAP_FROM_BE);

    return 0;
}

int mrfiocDBuff_write(
        regDevice* device,
        size_t offset,
        unsigned int datalength,
        size_t nelem,
        void* pdata,
        void* pmask,
        int priority,
        regDevTransferComplete callback,
        char* user)
{
    if (!device->txBuffer) {
        errlogPrintf(
                "mrfiocDBuff_write %s: FATAL ERROR! txBuffer not allocated!\n", user);
        return -1;
    }

    /*
     * We use offset 0 (that is illegal for normal use since it is occupied by protoID)
     * to flush the output buffer. This eliminates the need for extra record..
     */
    if (offset == 0) {
        mrfiocDBuff_flush(device);
        return 0;
    }

    if (offset < PROTO_LEN) {
        errlogPrintf(
                "mrfiocDBuff_write %s: device %s: byte offset must be greater than %d\n",
                user, device->name, PROTO_LEN);
        return -1;
    }

    //Copy into the scratch buffer
        /* Data in buffer is in big endian byte order */
    regDevCopy(datalength, nelem, pdata, &device->txBuffer[offset], pmask, REGDEV_SWAP_TO_BE);

    //Update buffer length (rounded up to multiple of 4)
    size_t bufferLen = (datalength * nelem + offset + 3) & ~3;
    if (bufferLen > device->usedTxBufferLen) device->usedTxBufferLen = bufferLen;

    return 0;
}


IOSCANPVT mrfiocDBuff_getInIoscan(regDevice* device, size_t offset)
{
    return device->ioscanpvt;
}


//RegDev device definition
static const regDevSupport mrfiocDBuffSupport = {
    mrfiocDBuff_report,
    mrfiocDBuff_getInIoscan,
    NULL,
    mrfiocDBuff_read,
    mrfiocDBuff_write
};


/*
 * Initialization, this is the entry point.
 * Function is called from iocsh. Function will try
 * to find desired device (mrfName) and attach mrfiocDBuff
 * support to it.
 *
 * Args:Can not find mrf device: %s
 *         regDevName - desired name of the regDev device
 *         mrfName - name of mrfioc2 device (evg, evr, ...)
 *
 */

void mrfiocDBuffConfigure(const char* regDevName, const char* mrfName, int protocol)
{
    if (!regDevName || !mrfName) {
        errlogPrintf("usage: mrfiocDBuffConfigure \"regDevName\", \"mrfName\", [protocol]\n");
        return;
    }

    //Check if device already exists:
    if (regDevFind(regDevName)) {
        errlogPrintf("mrfiocDBuffConfigure: FATAL ERROR! device %s already exists!\n", regDevName);
        return;
    }

    regDevice* device;

    device = (regDevice*) calloc(1, sizeof(regDevice) + strlen(regDevName) + 1);
    if (!device) {
        errlogPrintf("mrfiocDBuffConfigure %s: FATAL ERROR! Out of memory!\n", regDevName);
        return;
    }
    device->name = (char*)(device + 1);
    strcpy(device->name, regDevName);

    /*
     * Query mrfioc2 device support for device
     */
    device->bufferHandle = mrmBufInit(mrfName);

    if (!device->bufferHandle) {
        errlogPrintf("mrfiocDBuffConfigure %s: FAILED! Can not connect to mrf device: %s\n", regDevName, mrfName);
        return;
    }

    epicsUInt32 maxLength;
    if (mrmBufMaxLen(device->bufferHandle, &maxLength) != 0) {
        /* don't know how much to allocate */
        printf ("mrfiocDBuffConfigure %s: mrmBufMaxLen() failed", regDevName);
        return;
    }
    dbgPrintf("mrfiocDBuffConfigure %s: %s buffer size is %u bytes\n",
        regDevName, mrfName, maxLength);

    device->proto = (epicsUInt32) protocol; //protocol ID
    epicsPrintf("mrfiocDBuffConfigure %s: registering to protocol %d\n", regDevName, device->proto);

    if (mrmBufTxSupported(device->bufferHandle))
    {
        dbgPrintf("mrfiocDBuffConfigure %s: %s supports TX buffer. Allocating.\n",
            regDevName, mrfName);
        /* Allocate the buffer memory */
        device->txBuffer = (epicsUInt8*) calloc(1, maxLength);
        if (!device->txBuffer) {
            errlogPrintf("mrfiocDBuffConfigure %s: FATAL ERROR! Could not allocate TX buffer!", regDevName);
            return;
        }
    }

    if (mrmBufRxSupported(device->bufferHandle))
    {
        dbgPrintf("mrfiocDBuffConfigure %s: %s supports RX buffer. Allocating and installing callback\n",
            regDevName, mrfName);
        device->rxBuffer = (epicsUInt8*) calloc(1, maxLength);
        if (!device->rxBuffer) {
            errlogPrintf("mrfiocDBuffConfigure %s: FATAL ERROR! Could not allocate RX buffer!", regDevName);
            return;
        }
        if (mrmBufRegCallback(device->bufferHandle, mrmEvrDataRxCB, device) == 0) {
            scanIoInit(&device->ioscanpvt);
        } else {
            errlogPrintf("mrfiocDBuffConfigure %s: buffer does not support callbacks\n", regDevName);
        }
    }

    regDevRegisterDevice(regDevName, &mrfiocDBuffSupport, device, maxLength);
}

/****************************************/
/*        EPICS IOCSH REGISTRATION        */
/****************************************/

/*         mrfiocDBuffConfigure           */
static const iocshArg mrfiocDBuffConfigureDefArg0 = { "regDevName", iocshArgString};
static const iocshArg mrfiocDBuffConfigureDefArg1 = { "mrfioc2 device name", iocshArgString};
static const iocshArg mrfiocDBuffConfigureDefArg2 = { "protocol", iocshArgInt};
static const iocshArg *const mrfiocDBuffConfigureDefArgs[3] = {&mrfiocDBuffConfigureDefArg0, &mrfiocDBuffConfigureDefArg1, &mrfiocDBuffConfigureDefArg2};

static const iocshFuncDef mrfiocDBuffConfigureDef = {"mrfiocDBuffConfigure", 3, mrfiocDBuffConfigureDefArgs};

static void mrfioDBuffConfigureFunc(const iocshArgBuf* args) {
    mrfiocDBuffConfigure(args[0].sval, args[1].sval, args[2].ival);
}


/*         mrfiocDBuffFlush           */
static const iocshArg mrfiocDBuffFlushDefArg0 = { "regDevName", iocshArgString};
static const iocshArg *const mrfiocDBuffFlushDefArgs[1] = {&mrfiocDBuffFlushDefArg0};

static const iocshFuncDef mrfiocDBuffFlushDef = {"mrfiocDBuffFlush", 1, mrfiocDBuffFlushDefArgs};

static void mrfioDBuffFlushFunc(const iocshArgBuf* args) {
    regDevice* device = regDevFind(args[0].sval);
    if(!device){
        errlogPrintf("Can not find device: %s\n", args[0].sval);
        return;
    }

    mrfiocDBuff_flush(device);
}

/*        registrar            */

static int mrfiocDBuffRegistrar(void) {
    iocshRegister(&mrfiocDBuffConfigureDef, mrfioDBuffConfigureFunc);
    iocshRegister(&mrfiocDBuffFlushDef, mrfioDBuffFlushFunc);

    return 1;
}
//Automatic registration
extern "C" {
 epicsExportRegistrar(mrfiocDBuffRegistrar);
}
static int done = mrfiocDBuffRegistrar();
