
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <epicsTypes.h>
#include <errlog.h>
#include <devLibPCI.h>

#include "mrmpci.h"

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
    epicsUInt32 i;
    args *a=raw;
    volatile epicsUInt32 *base;

    devPCIShowDevice(2,dev);

    if (devPCIToLocalAddr(dev, a->bar, (volatile void**)(void *)&base, 0)) {
        fprintf(stderr,"Failed to map bar\n");
        return 0;
    }

    for (i=a->offset/4; i<(a->offset+a->len)/4; i++) {
        if(i%4==0)
            printf("\n%08x : ", 4*i);

        printf("%08x ",base[i]);

    }
    printf("\n");
    return 0;
}

int main(int argc, char* argv[])
{
    extern void devLibPCIRegisterBaseDefault(void);
    devLibPCIRegisterBaseDefault();

    fprintf(stderr,"Usage: evrdump [BAR#] [Length] [offset]\n");
    args a = {0,0x80,0};

    if (argc>=2) a.bar=atoi(argv[1]);
    if (argc>=3) a.len=atoi(argv[2]);
    if (argc>=4) a.offset=atoi(argv[3]);

    devPCIFindCB(mrmevrs, &printevr, &a, 0);

    return 0;
}
