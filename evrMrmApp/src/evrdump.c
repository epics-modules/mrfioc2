/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <epicsTypes.h>
#include <errlog.h>
#include <devLibPCI.h>

#include "mrmpci.h"

#ifndef _WIN32
epicsShareFunc void devLibPCIRegisterBaseDefault(void);
#endif

static const epicsPCIID mrmevrs[] = {
   DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9030,    PCI_VENDOR_ID_PLX,
                              PCI_DEVICE_ID_MRF_PMCEVR_230, PCI_VENDOR_ID_MRF)
  ,DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9030,    PCI_VENDOR_ID_PLX,
                              PCI_DEVICE_ID_MRF_PXIEVR_230, PCI_VENDOR_ID_MRF)
  ,DEVPCI_END
};

typedef struct {
    int bar, len, offset;
} args;

int printevr(void* raw,const epicsPCIDevice* dev)
{
    epicsUInt32 i, offset, length;
    args *a=raw;
    volatile epicsUInt32 *base;

    devPCIShowDevice(2,dev);

    if (devPCIToLocalAddr(dev, a->bar, (volatile void**)(void *)&base, DEVLIB_MAP_UIO1TO1)) {
        fprintf(stderr,"Failed to map bar\n");
        return 0;
    }

    offset = (epicsUInt32)a->offset;
    length = (epicsUInt32)a->len;

    for (i=offset/4; i < (offset + length)/4; i++) {
        if(i%4==0)
            printf("\n%08x : ", 4*i);

        printf("%08x ",base[i]);

    }
    printf("\n");
    return 0;
}

int main(int argc, char* argv[])
{
    args a = {0,0x80,0};

#ifndef _WIN32
    devLibPCIRegisterBaseDefault();
#endif

    fprintf(stderr,"Usage: evrdump [BAR#] [Length] [offset]\n");

    if (argc>=2) a.bar=atoi(argv[1]);
    if (argc>=3) a.len=atoi(argv[2]);
    if (argc>=4) a.offset=atoi(argv[3]);

    devPCIFindCB(mrmevrs, &printevr, &a, 0);

    return 0;
}
