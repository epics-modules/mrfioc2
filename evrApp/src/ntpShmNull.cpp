#include <stdio.h>
#include <drvSup.h>

static void ntpShmRegister()
{
    // nothing to do here...
}

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
epicsExportRegistrar(ntpShmRegister);
