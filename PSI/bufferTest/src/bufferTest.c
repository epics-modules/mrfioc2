#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <ellLib.h>
#include <iocsh.h>

#include <epicsThread.h>
#include <epicsExport.h>
#include <epicsExit.h>

#include "../evrMrmApp/src/buf.h"

/* Subtract member byte offset, returning pointer to parent object */
#ifndef CONTAINER
# ifdef __GNUC__
#   define CONTAINER(ptr, structure, member) ({                     \
        const __typeof(((structure*)0)->member) *_ptr = (ptr);      \
        (structure*)((char*)_ptr - offsetof(structure, member));    \
    })
# else
#   define CONTAINER(ptr, structure, member) \
        ((structure*)((char*)(ptr) - offsetof(structure, member)))
# endif
#endif

#define THREAD_SEND     "BufferTestSend"
#define BUFFER_SIZE     5

typedef struct deviceData {
    char *deviceName;

    epicsThreadId *threadId;
    int running;

    bufferInfo_t *bufferData;

    ELLNODE node;
} deviceData_t;

static ELLLIST *devices = NULL;

static void threadRun(void *param)
{
    int i;
    epicsStatus rc;
    deviceData_t *devData = (deviceData_t *)param;
    epicsUInt8 *buffer = NULL;
    epicsUInt32 size;
    time_t curTime;

    if(!devData->bufferData) {
        devData->bufferData = bufInit(devData->deviceName);
    }

    if(!devData->bufferData) {
        printf("ERROR: Buffer data not initialized!\n");
        return;
    }

    buffer = calloc(BUFFER_SIZE, sizeof(epicsUInt8));
    if(!buffer) {
        printf("ERROR: Unable to allocate data for buffer!\n");
        return;
    }

//    rc = bufEnable(devData->bufferData);
//    if(rc) {
//        printf("ERROR: Failiure to enable data buffers!\n");
//        return;
//    }

    devData->running = 1;

    while(devData->running) {
        curTime = time(NULL);

        ((time_t *)buffer)[0] = time(NULL);
        buffer[4] = 0;
        size = BUFFER_SIZE;

        rc = bufSend(devData->bufferData, size, buffer);
        if(rc) {
            printf("ERROR: Failed to send buffer data!\n");
        }

        // Sleep for a second unless we are stopping the thread
        for(i=0; i < 10 && devData->running; i++)
            epicsThreadSleep(0.1);
    }
}

static void recievedCallback(void *arg, epicsStatus status, epicsUInt32 length, const epicsUInt8 *buffer) {

    int i;
    char *deviceName = (char *)arg;

    printf("DEV: %s, STATUS: %s, BUF:", deviceName, status==0?"OK":"NOK");
    for(i = 0; i < length; i++) {
        printf(" 0x%02x", buffer[i]);
    }
    printf("\n");
}

static void bufferMonitor(char *deviceName) {

    epicsStatus rc;
    deviceData_t *devData;
    ELLNODE *node;
    int found = 0;

    for(node = ellFirst(devices); node; node = ellNext(node))
    {
        devData = CONTAINER(node, deviceData_t, node);
        // Don't add duplicates
        if (strcmp(devData->deviceName, deviceName) == 0) {
            found = 1;
            break;
        }
    }

    // Create new device data and add to list
    if(!found) {
        devData = (deviceData_t *)calloc(1, sizeof(deviceData_t));
        if(!devData) {
            printf("bufferSend ERROR: Failed to allocate memory for device data.");
            return;
        }

        devData->deviceName = strdup(deviceName);

        ellAdd(devices, &devData->node);
    }

    if(!devData->bufferData) {
        devData->bufferData = bufInit(deviceName);
    }

    if(!devData->bufferData) {
        printf("ERROR: Buffer data not initialized!\n");
        return;
    }

    rc = bufRegCallback(devData->bufferData, recievedCallback, (void *)devData->deviceName);
    if(rc) {
        printf("ERROR: Failed to register callback!\n");
    } else {
        printf("Callback successfully regisrered!\n");

//        rc = bufEnable(devData->bufferData);
//        if(rc) {
//            printf("ERROR: Failiure to enable data buffers!\n");
//            return;
//        }
    }
}

static void bufferSend(char *deviceName) {

    deviceData_t *devData = NULL;
    ELLNODE *node;
    int found = 0;

    for(node = ellFirst(devices); node; node = ellNext(node))
    {
        devData = CONTAINER(node, deviceData_t, node);
        // Don't add duplicates
        if (strcmp(devData->deviceName, deviceName) == 0) {
            found = 1;
            break;
        }
    }

    // Create new device data and add to list
    if(!found) {
        devData = (deviceData_t *)calloc(1, sizeof(deviceData_t));
        if(!devData) {
            printf("bufferSend ERROR: Failed to allocate memory for device data.");
            return;
        }

        devData->deviceName = strdup(deviceName);

        ellAdd(devices, &devData->node);
    }

    // Start device send thread
    if(!devData->threadId) {
        devData->threadId = epicsThreadMustCreate(THREAD_SEND,
            epicsThreadPriorityLow,
            epicsThreadGetStackSize(epicsThreadStackSmall),
            threadRun,
            (void *)devData);

        printf("Send thread started for device %s!\n", devData->deviceName);
    } else {
        printf("WARNING: Send thread already started for device %s!\n", devData->deviceName);
    }
}

static void bufferTestExit(void *arg) {

    deviceData_t *devData;
    ELLNODE *node;

    for(node = ellFirst(devices); node; node = ellNext(node))
    {
        devData = CONTAINER(node, deviceData_t, node);
        devData->running = 0;
    }
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
    devices = (ELLLIST *)malloc(sizeof(ELLLIST));
    if(!devices) {
        printf("bufferTestRegister ERROR: Failed to initialize list.");
        return;
    }

    ellInit(devices);

    iocshRegister(&epicsBuffMonFuncDef, epicsBuffMonCallFunc);
    iocshRegister(&epicsBuffSendFuncDef, epicsBuffSendCallFunc);

    epicsAtExit(bufferTestExit, NULL);
}
epicsExportRegistrar(bufferTestRegister);
