#include <stdio.h>
#include <dbCommon.h>
#include <drvSup.h>
#include <alarm.h>
#include <recGbl.h>
#include <devSup.h>
#include <drvSup.h>

static void ntpShmRegister()
{
    // nothing to do here...
}

static long init_record(dbCommon*) { return 0; }

static long read_record(dbCommon* prec)
{
    recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
    return 0;
}

typedef struct {
    dset common;
    DEVSUPFUN read_fn;
    DEVSUPFUN lin_convert;
} commonset;

static commonset devNtpShmLiOk = {
    {6, NULL, NULL, (DEVSUPFUN)&init_record, NULL},
    (DEVSUPFUN)&read_record,
    NULL
};

static commonset devNtpShmLiFail = {
    {6, NULL, NULL, (DEVSUPFUN)&init_record, NULL},
    (DEVSUPFUN)&read_record,
    NULL
};

static commonset devNtpShmAiDelta = {
    {6, NULL, NULL, (DEVSUPFUN)&init_record, NULL},
    (DEVSUPFUN)&read_record,
    NULL
};

static void ntpShmReport(int)
{
    fprintf(stderr, "Not implemented for this target\n");
}

static drvet ntpShared = {
    2,
    (DRVSUPFUN)&ntpShmReport,
    0,
};

#include <epicsExport.h>

epicsExportAddress(drvet, ntpShared);
epicsExportAddress(dset, devNtpShmLiOk);
epicsExportAddress(dset, devNtpShmLiFail);
epicsExportAddress(dset, devNtpShmAiDelta);
epicsExportRegistrar(ntpShmRegister);
