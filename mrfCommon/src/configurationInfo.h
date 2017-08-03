/*************************************************************************\
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef CONFIGURATIONINFO_H
#define CONFIGURATIONINFO_H

#include <devLibPCI.h>

//VME
struct configuration_vme{
    epicsInt32 slot;        // slot where the card is inserted
    epicsUInt32 address;    // VME address in A24 space
    epicsInt32 irqLevel;    // interupt level
    epicsInt32 irqVector;   // interrupt vector
    std::string position;   // position description for EVR
};


// PCI
struct configuration_pci{
    const epicsPCIDevice *dev;
    configuration_pci()
        :dev(0)
    {}
};

enum busType{
    busType_vme = 0,
    busType_pci = 1
};

struct bus_configuration{
    struct configuration_vme vme;
    struct configuration_pci pci;
    enum busType busType;
};

// form factor corresponds to FPGA Firmware Version Register bit 26-24
enum formFactor {
  formFactor_unknown = -1,
  formFactor_CPCI=0, // 3U
  formFactor_PMC=1,
  formFactor_VME64=2,
  formFactor_CRIO=3,
  formFactor_CPCIFULL=4, // 6U
  formFactor_PXIe=6,
  formFactor_PCIe=7,
  formFactor_mTCA=8,
};

#endif // CONFIGURATIONINFO_H
