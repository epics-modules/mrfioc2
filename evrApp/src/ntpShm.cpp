/*************************************************************************\
* Copyright (c) 2013 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Serve up EVR time to the shared memory driver (#28) of the NTP daemon.
 *
 * cf. http://www.eecis.udel.edu/~mills/ntp/html/drivers/driver28.html
 *
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 *
 * To use, add to init script.  Where 0<=N<=4.  To use 0 or 1 the IOC
 * must run as root.
 *
 *   time2ntp("evrname", N)
 *
 * Add to NTP daemon config.  Replace 'prefer' with 'noselect' when testing
 *
 *   server 127.127.28.N minpoll 1 maxpoll 2 prefer
 *   fudge 127.127.28.N refid EVR
 *
 * Order of execution in this file.
 * 1) User calls time2ntp() before iocInit()
 * 2) ntpshmhooks() is called during iocInit()
 * 3) ntpsetup() is called periodically until it succeeds
 * 4) ntpshmupdate() is called once per second.
 */
#include <stdio.h>
#include <errno.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>

#include <epicsTime.h>
#include <epicsVersion.h>
#include <epicsMutex.h>
#include <epicsThread.h>
#include <epicsStdio.h> /* redirects stdout/err */
#include <callback.h>
#include <drvSup.h>
#include <recGbl.h>
#include <longinRecord.h>
#include <aiRecord.h>
#include <menuConvert.h>
#include <dbScan.h>
#include <iocsh.h>
#include <initHooks.h>

#include "evr/evr.h"

// NTPD itself doesn't make much attempt at synchronizing
// but we try to do better
#if defined(__GNUC__)
#  if ( ( __GNUC__ * 100 + __GNUC_MINOR__ ) >= 401 )
#    define SYNC() __sync_synchronize()
#  else
#    define SYNC() __asm__ __volatile__ ("":::"memory")
#  endif
#else
   // oh well, we tried
#  define SYNC() do{}while(0)
#endif

// from a shell run 'ipcs -m' while NTPD is running w/ driver28 loaded.
#define NTPD_SEG0 0x4E545030

#define RETRY_TIME 30.0

// definition of shared segment, as described in
// http://www.eecis.udel.edu/~mills/ntp/html/drivers/driver28.html
typedef struct {
    int mode;
    int count;

    // The EVR timestamp
    time_t stampSec;
    int stampUsec;

    // The system time when it was acquired
    time_t rxSec;
    int rxUsec;

    int leap;
    int precision;
    int nsamples;
    int valid;

    int pad[10];
} shmSegment;

static epicsThreadOnceId ntponce = EPICS_THREAD_ONCE_INIT;

typedef struct {

    epicsMutexId ntplock;

    CALLBACK ntp_cb;

    epicsUInt32 event;

    EVR *evr;
    int segid;

    shmSegment* seg;

    int notify_nomap;
    int notify_1strx;

    IOSCANPVT lastUpdate;

    bool lastValid;
    epicsTimeStamp lastStamp;
    epicsTimeStamp lastRx;

    unsigned int numOk;
    unsigned int numFail;
} ntpShmPriv;
static ntpShmPriv ntpShm;


static void incFail()
{
    epicsMutexMustLock(ntpShm.ntplock);
    ntpShm.lastValid = false;
    ntpShm.numFail++;
    epicsMutexUnlock(ntpShm.ntplock);
}

static void ntpshmupdate(void*, epicsUInt32 event)
{
    if(event!=ntpShm.event) {
        incFail(); return;
    }

    epicsTimeStamp evrts;
    if(!ntpShm.evr->getTimeStamp(&evrts, 0)) // read current wall clock time
    {
        // no valid device time
        incFail(); return;
    }

    struct timeval cputs;
    if(gettimeofday(&cputs, 0))
    {
        // no valid cpu time?
        incFail(); return;
    }

    struct timeval evrts_posix;
    evrts_posix.tv_sec = evrts.secPastEpoch + POSIX_TIME_AT_EPICS_EPOCH;
    evrts_posix.tv_usec = evrts.nsec / 1000;
    // correct for bias in truncation
    if(evrts.nsec % 1000 >= 500) {
        evrts_posix.tv_usec += 1;
        if(evrts_posix.tv_usec>=1000000) {
            evrts_posix.tv_sec += 1;
            evrts_posix.tv_usec = 0;
        }
    }

    // volatile operations aren't really enough, but will have to do.
    volatile shmSegment* seg=ntpShm.seg;

    seg->valid = 0;
    SYNC();
    int c1 = seg->count++;
    seg->stampSec = evrts_posix.tv_sec;
    seg->stampUsec = evrts_posix.tv_usec;
    seg->rxSec = cputs.tv_sec;
    seg->rxUsec = cputs.tv_usec;
    int c2 = seg->count++;
    if(c1+1!=c2) {
        fprintf(stderr, "ntpshmupdate: possible collision with another writer!\n");
        incFail(); return;
    }
    seg->valid = 1;
    SYNC();

    epicsMutexMustLock(ntpShm.ntplock);
    ntpShm.lastValid = true;
    ntpShm.numOk++;
    ntpShm.lastStamp = evrts;
    ntpShm.lastRx = epicsTime(cputs);
    epicsMutexUnlock(ntpShm.ntplock);

    scanIoRequest(ntpShm.lastUpdate);

    if(!ntpShm.notify_1strx) {
        fprintf(stderr, "First update ready for NTPD\n");
        ntpShm.notify_1strx = 1;
    }

    return; // normal exit
}

static void ntpsetup(CALLBACK *)
{
    // We don't set IPC_CREAT, but instead wait for NTPD to start and initialize
    // as it wants
    int mode = ntpShm.segid <=1 ? 0600 : 0666;

    int shmid = shmget((key_t)(NTPD_SEG0+ntpShm.segid), sizeof(shmSegment), mode);

    if(shmid==-1) {
        if(errno==ENOENT) {
            if(!ntpShm.notify_nomap) {
                fprintf(stderr, "Can't find shared memory segment.  Either NTPD hasn't started,"
                        " or is not configured correctly.  Will retry later.");
                ntpShm.notify_nomap = 1;
            }
            callbackRequestDelayed(&ntpShm.ntp_cb, RETRY_TIME);
        } else {
            perror("ntpshmsetup: shmget");
        }
        return;
    }

    ntpShm.seg = (shmSegment*)shmat(shmid, 0, 0);
    if(ntpShm.seg==(shmSegment*)-1) {
        perror("ntpshmsetup: shmat");
        return;
    }

    ntpShm.seg->mode = 1;
    ntpShm.seg->valid = 0;
    SYNC();
    ntpShm.seg->leap = 0; //TODO: what does this do?
    ntpShm.seg->precision = -19; // pow(2,-19) ~= 1e-6 sec
    ntpShm.seg->nsamples = 3; //TODO: what does this do?
    SYNC();

    try {
        ntpShm.evr->eventNotifyAdd(ntpShm.event, &ntpshmupdate, 0);
    } catch(std::exception& e) {
        fprintf(stderr, "Error registering for 1Hz event: %s\n", e.what());
    }
}

static void ntpshminit(void*)
{
    ntpShm.ntplock = epicsMutexMustCreate();

    callbackSetPriority(priorityLow, &ntpShm.ntp_cb);
    callbackSetCallback(&ntpsetup, &ntpShm.ntp_cb);
    callbackSetUser(0, &ntpShm.ntp_cb);
}

static void ntpshmhooks(initHookState state)
{
    if(state!=initHookAfterIocRunning)
        return;

    epicsThreadOnce(&ntponce, &ntpshminit, 0);

    epicsMutexMustLock(ntpShm.ntplock);
    if(ntpShm.evr) {
        callbackRequest(&ntpShm.ntp_cb);
        fprintf(stderr, "Starting NTP SHM writer for segment %d\n", ntpShm.segid);
    }
    epicsMutexUnlock(ntpShm.ntplock);
}

void time2ntp(const char* evrname, int segid, int event)
{
    try {
        if(event==0)
            event = MRF_EVENT_TS_COUNTER_RST;
        else if(event<=0 || event >255) {
            fprintf(stderr, "Invalid 1Hz event # %d\n", event);
            return;
        }
        if(segid<0 || segid>4) {
            fprintf(stderr, "Invalid segment ID %d\n", segid);
            return;
        }
        mrf::Object *obj = mrf::Object::getObject(evrname);
        if(!obj) {
            fprintf(stderr, "Unknown EVR: %s\n", evrname);
            return;
        }

        EVR *evr = dynamic_cast<EVR*>(obj);
        if(!evr) {
            fprintf(stderr, "\"%s\" is not an EVR\n", evrname);
            return;
        }

        epicsThreadOnce(&ntponce, &ntpshminit, 0);

        epicsMutexMustLock(ntpShm.ntplock);

        if(ntpShm.evr) {
            epicsMutexUnlock(ntpShm.ntplock);
            fprintf(stderr, "ntpShm already initialized.\n");
            return;
        }

        ntpShm.event = event;
        ntpShm.evr = evr;
        ntpShm.segid = segid;

        epicsMutexUnlock(ntpShm.ntplock);
    } catch(std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
    }
}

static const iocshArg time2ntpArg0 = { "evr name",iocshArgString};
static const iocshArg time2ntpArg1 = { "NTP segment id",iocshArgInt};
static const iocshArg time2ntpArg2 = { "1Hz Event code",iocshArgInt};
static const iocshArg * const time2ntpArgs[3] =
{&time2ntpArg0,&time2ntpArg1,&time2ntpArg2};
static const iocshFuncDef time2ntpFuncDef =
    {"time2ntp",3,time2ntpArgs};
static void time2ntpCallFunc(const iocshArgBuf *args)
{
    time2ntp(args[0].sval,args[1].ival,args[2].ival);
}

static long init_record(dbCommon*) { return 0; }

static long get_ioint_info(int /*cmd*/, dbCommon */*pRec*/, IOSCANPVT *ppvt)
{
    *ppvt = ntpShm.lastUpdate;
    return 0;
}

static long read_ok(longinRecord* prec)
{
    epicsMutexMustLock(ntpShm.ntplock);
    prec->val = ntpShm.numOk;
    epicsMutexUnlock(ntpShm.ntplock);
    return 0;
}

static long read_fail(longinRecord* prec)
{
    epicsMutexMustLock(ntpShm.ntplock);
    prec->val = ntpShm.numFail;
    epicsMutexUnlock(ntpShm.ntplock);
    return 0;
}

static long read_delta(aiRecord* prec)
{
    epicsMutexMustLock(ntpShm.ntplock);
    double val = 0.0;
    if(ntpShm.lastValid)
        val = epicsTimeDiffInSeconds(&ntpShm.lastStamp, &ntpShm.lastRx);
    else
        recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
    if(prec->tse==epicsTimeEventDeviceTime) {
        prec->time = ntpShm.lastStamp;
    }
    epicsMutexUnlock(ntpShm.ntplock);

    if(prec->linr==menuConvertLINEAR){
        val-=prec->eoff;
        if(prec->eslo!=0)
            val/=prec->eslo;
    }
    val-=prec->aoff;
    if(prec->aslo!=0)
        val/=prec->aslo;
    prec->val = val;
    prec->udf = !isfinite(val);

    return 2;
}

static void ntpShmReport(int)
{
    epicsMutexMustLock(ntpShm.ntplock);
    EVR *evr=ntpShm.evr;
    unsigned int ok=ntpShm.numOk,
                 fail=ntpShm.numFail;
    epicsMutexUnlock(ntpShm.ntplock);

    if(evr) {
        printf("Driver is active\n ok#: %u\n fail#: %u\n", ok, fail);
    } else {
        printf("Driver is not active\n");
    }
}

static void ntpShmInit()
{
    scanIoInit(&ntpShm.lastUpdate);
}

static void ntpShmRegister()
{
    initHookRegister(&ntpshmhooks);
    iocshRegister(&time2ntpFuncDef,&time2ntpCallFunc);
}

typedef struct {
    dset common;
    DEVSUPFUN read_fn;
    DEVSUPFUN lin_convert;
} commonset;

static commonset devNtpShmLiOk = {
    {6, NULL, NULL, (DEVSUPFUN)&init_record, (DEVSUPFUN)&get_ioint_info},
    (DEVSUPFUN)&read_ok,
    NULL
};

static commonset devNtpShmLiFail = {
    {6, NULL, NULL, (DEVSUPFUN)&init_record, (DEVSUPFUN)&get_ioint_info},
    (DEVSUPFUN)&read_fail,
    NULL
};

static commonset devNtpShmAiDelta = {
    {6, NULL, NULL, (DEVSUPFUN)&init_record, (DEVSUPFUN)&get_ioint_info},
    (DEVSUPFUN)&read_delta,
    NULL
};

static drvet ntpShared = {
    2,
    (DRVSUPFUN)&ntpShmReport,
    (DRVSUPFUN)&ntpShmInit,
};

#include <epicsExport.h>

extern "C"{
 epicsExportAddress(drvet, ntpShared);
 epicsExportAddress(dset, devNtpShmLiOk);
 epicsExportAddress(dset, devNtpShmLiFail);
 epicsExportAddress(dset, devNtpShmAiDelta);
 epicsExportRegistrar(ntpShmRegister);
}
