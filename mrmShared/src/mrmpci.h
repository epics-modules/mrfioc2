/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef MRMPCI_H
#define MRMPCI_H

#define PCI_VENDOR_ID_XILINX          0x10ee

#define PCI_DEVICE_ID_XILINX_DEV      0x7011

#define PCI_VENDOR_ID_PLX             0x10b5   /* PCI Vendor ID for PLX Technology, Inc.          */

#define PCI_DEVICE_ID_PLX_9030        0x9030   /* PCI Device ID for PLX-9030 bridge chip          */
#define PCI_DEVICE_ID_PLX_9056        0x9056

#define PCI_VENDOR_ID_LATTICE         0x1204

#define PCI_DEVICE_ID_EC_30           0xEC30

#define PCI_VENDOR_ID_MRF             0x1a3e   /* PCI Vendor ID for Micro Research Finland, Oy    */

#define PCI_DEVICE_ID_MRF_PMCEVR_200   0x10c8   /* PCI Device ID for MRF PMC-EVR-200              */

#define PCI_DEVICE_ID_MRF_PXIEVR_220   0x10dc   /* PCI Device ID for MRF PXI-EVR-220              */

#define PCI_DEVICE_ID_MRF_PMCEVR_230   0x11e6   /* PCI Device ID for MRF PMC-EVR-230              */
#define PCI_DEVICE_ID_MRF_PXIEVR_230   0x10e6   /* PCI Device ID for MRF PXI-EVR-230              */
#define PCI_DEVICE_ID_MRF_EVRTG_300    0x192c   /* PCI Device ID for MRF PCI-EVRTG-300            */
#define PCI_DEVICE_ID_MRF_EVRTG_300E   0x172c   /* PCI Device ID for MRF PCI-EVRTG-300            */
/* PCIe-EVR-300 and PCIe-EVR-300DC */
#define PCI_SUBDEVICE_ID_PCIE_EVR_300       0x172c
/* mTCA-EVR-300 */
#define PCI_DEVICE_ID_MRF_EVRMTCA300  0x132c

/* mTCA-EVM-300 */
#define PCI_DEVICE_ID_MRF_MTCA_EVM_300      0x232c

#define PCI_DEVICE_ID_MRF_PXIEVG230   0x20E6
/* cPCI-EVG-220 */
#define PCI_SUBDEVICE_ID_MRF_PXIEVG_220     0x20dc

#define PCI_DEVICE_ID_MRF_CPCIEVG300 0x252c
#define PCI_DEVICE_ID_MRF_CPCIEVR300 0x152c

#endif /* MRMPCI_H */
