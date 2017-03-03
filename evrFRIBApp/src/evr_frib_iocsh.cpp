/*
 * This software is Copyright by the Board of Trustees of Michigan
 * State University (c) Copyright 2017.
 *
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include <memory>

#include <iocsh.h>
#include <epicsExport.h>

#include <devLibPCI.h>

#include "evr_frib.h"
#include "evrFRIBRegMap.h"

static const epicsPCIID evr_frib_ids[] = {
    DEVPCI_SUBDEVICE_SUBVENDOR(0xfdbe, 0x10ee, 0x1234, 0x10ee),
//  BPM
//    DEVPCI_SUBDEVICE_SUBVENDOR(0xfbad, 0x10ee, 0x1234, 0x10ee),
    DEVPCI_END
};

void fribEvrSetupPCI(const char *name, const char *pcispec)
{
    try {
        const epicsPCIDevice *dev = NULL;

        if(devPCIFindSpec(evr_frib_ids, pcispec, &dev, 0)) {
            fprintf(stderr, "No such device: %s\n", pcispec);
            return;
        }
        fprintf(stderr, "Found device\n");

        volatile unsigned char *base = NULL;
        if(devPCIToLocalAddr(dev, 0, (volatile void**)&base, 0)) {
            fprintf(stderr, "Can't map BAR 0 of %s\n", pcispec);
            return;
        }

        epicsUInt32 blen = 0;
        if(devPCIBarLen(dev, 0, &blen)) {
            fprintf(stderr, "Can't determine BAR 0 len of %s\n", pcispec);
            return;
        }

        if(!base || blen<REGLEN) {
            fprintf(stderr, "Invalid base %p or length %u of %s\n", base, (unsigned)blen, pcispec);
            return;
        }

        bus_configuration bconf;
        bconf.busType = busType_pci;
        bconf.pci.domain = dev->domain;
        bconf.pci.bus = dev->bus;
        bconf.pci.device = dev->device;
        bconf.pci.function = dev->function;

        std::auto_ptr<EVRFRIB> evr(new EVRFRIB(name, bconf, base));


        // lose our pointer
        // a pointer is retained in the global Objects list
        evr.release();

        fprintf(stderr, "Ready\n");
    }catch(std::exception &e){
        fprintf(stderr, "Error: %s\n", e.what());
    }
}


static const iocshArg fribEvrSetupPCIArg0 = { "name",iocshArgString};
static const iocshArg fribEvrSetupPCIArg1 = { "PCI id",iocshArgString};
static const iocshArg * const fribEvrSetupPCIArgs[2] =
{&fribEvrSetupPCIArg0,&fribEvrSetupPCIArg1};
static const iocshFuncDef fribEvrSetupPCIFuncDef =
    {"fribEvrSetupPCI",2,fribEvrSetupPCIArgs};
static void fribEvrSetupPCICallFunc(const iocshArgBuf *args)
{
    fribEvrSetupPCI(args[0].sval,args[1].sval);
}

static
void frib_evr_register()
{
    iocshRegister(&fribEvrSetupPCIFuncDef,fribEvrSetupPCICallFunc);
}

extern "C" {
epicsExportRegistrar(frib_evr_register);
}
