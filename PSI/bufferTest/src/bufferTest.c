#include <stdlib.h>
#include <time.h>

#include <iocsh.h>

#include <epicsThread.h>
#include <epicsExport.h>

#include "../evrMrmApp/src/buf.h"

#define THREAD_SEND     "BufferTestSend"
#define BUFFER_SIZE     5

static epicsThreadId *threadId = NULL;
static bufferInfo_t *bufferDataTx = NULL;
static bufferInfo_t *bufferDataRx = NULL;
static int running = 1;

static void threadRun(void *param)
{
    int i;
    epicsStatus rc;
    char *deviceName = (char *)param;
    epicsUInt8 *buffer = NULL;
    epicsUInt32 size;
    time_t curTime;

    if(!bufferDataTx) {
        bufferDataTx = bufInit(deviceName);
    }

    if(!bufferDataTx) {
        printf("ERROR: Buffer data not initialized!\n");
        return;
    }

    buffer = calloc(BUFFER_SIZE, sizeof(epicsUInt8));
    if(!buffer) {
        printf("ERROR: Unable to allocate data for buffer!\n");
        return;
    }

//    rc = bufEnable(bufferDataTx);
//    if(rc) {
//        printf("ERROR: Failiure to enable data buffers!\n");
//        return;
//    }

    while(running) {
        curTime = time(NULL);

        buffer[0] = curTime & 0xFF;
        buffer[1] = (curTime >> 8) & 0xFF;
        buffer[2] = (curTime >> 16) & 0xFF;
        buffer[3] = (curTime >> 24) & 0xFF;
        buffer[4] = 0;
        size = 5;

        rc = bufSend(bufferDataTx, size, buffer);
        if(rc) {
            printf("ERROR: Failed to send buffer data!\n");
        }

        // Sleep for a second unless we are stopping the thread
        for(i=0; i < 10 && running; i++)
            epicsThreadSleep(0.1);
    }
}

static void recievedCallback(void *arg, epicsStatus status, epicsUInt32 length, const epicsUInt8 *buffer) {

    int i;
    char *deviceName = (char *)arg;

    printf("DEV: %s, STATUS: %s, BUF:", deviceName, status?"OK":"NOK");
    for(i = 0; i < length; i++) {
        printf(" 0x%X", buffer[i]);
    }
    printf("\n");
}

static void bufferMonitor(char *deviceName) {

    epicsStatus rc;

    if(!bufferDataRx) {
        bufferDataRx = bufInit(deviceName);
    }

    if(!bufferDataRx) {
        printf("ERROR: Buffer data not initialized!\n");
        return;
    }

    rc = bufRegCallback(bufferDataRx, recievedCallback, (void *)deviceName);
    if(rc) {
        printf("ERROR: Failed to register callback!\n");
    } else {
        printf("Callback successfully regisrered!\n");

//        rc = bufEnable(bufferDataTx);
//        if(rc) {
//            printf("ERROR: Failiure to enable data buffers!\n");
//            return;
//        }
    }
}

static void bufferSend(char *deviceName) {
    if(!threadId) {
        threadId = epicsThreadMustCreate(THREAD_SEND,
                epicsThreadPriorityLow,
                epicsThreadGetStackSize(epicsThreadStackSmall),
                threadRun,
                (void *)deviceName);

        printf("Send thread started!\n");
    } else {
        printf("WARNING: Send thread already started!\n");
    }
}

static void bufferTestExit(void *arg) {
    running = 0;
}

static const iocshArg epicsBuffMonArg0 = { "deviceNameMonitor",iocshArgString};
static const iocshArg *const epicsBuffMonArgs[1] = {&epicsBuffMonArg0};
static const iocshFuncDef const epicsBuffMonFuncDef = {"bufferMonitor",1,epicsBuffMonArgs};
static void epicsBuffMonCallFunc(const iocshArgBuf *args)
{
    bufferMonitor(args[0].sval);
}

static const iocshArg epicsBuffSendArg0 = { "deviceNameSend",iocshArgString};
static const iocshArg *const epicsBuffSendArgs[1] = {&epicsBuffSendArg0};
static const iocshFuncDef const epicsBuffSendFuncDef = {"bufferSend",1,epicsBuffSendArgs};
static void epicsBuffSendCallFunc(const iocshArgBuf *args)
{
    bufferSend(args[0].sval);
}

static void bufferTestRegister(void)
{
    iocshRegister(&epicsBuffMonFuncDef, epicsBuffMonCallFunc);
    iocshRegister(&epicsBuffSendFuncDef, epicsBuffSendCallFunc);

    epicsAtExit(bufferTestExit, NULL);
}
epicsExportRegistrar(bufferTestRegister);
