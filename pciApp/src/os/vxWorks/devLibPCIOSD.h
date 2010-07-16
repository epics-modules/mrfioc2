#ifndef OSDPCI_H_INC
#define OSDPCI_H_INC

#include <vxWorks.h>
#include <types.h>
#include <drv/pci/pciConfigLib.h>
#include <sysLib.h>
#include <vxLib.h>

/* The interfaces for PCI access on vxWorks and RTEMS
 * are almost identical, except for the names.
 *
 * Here we map the RTEMS names to their vxWorks
 * equivalent.
 */

#define pci_find_device pciFindDevice

#define pci_read_config_byte  pciConfigInByte
#define pci_read_config_word  pciConfigInWord
#define pci_read_config_dword pciConfigInLong

#define pci_write_config_byte  pciConfigOutByte
#define pci_write_config_word  pciConfigOutWord
#define pci_write_config_dword pciConfigOutLong


/* 0 <= N <= 5 */
#define PCI_BASE_ADDRESS(N) ( PCI_CFG_BASE_ADDRESS_0 + 4*(N) )

#define PCI_DEVICE_ID           PCI_CFG_DEVICE_ID
#define PCI_VENDOR_ID           PCI_CFG_VENDOR_ID
#define PCI_SUBSYSTEM_ID        PCI_CFG_SUB_SYSTEM_ID
#define PCI_SUBSYSTEM_VENDOR_ID PCI_CFG_SUB_VENDER_ID
#define PCI_CLASS_REVISION      PCI_CFG_REVISION

#define PCI_BASE_ADDRESS_SPACE    PCI_BAR_SPACE_MASK
#define PCI_BASE_ADDRESS_SPACE_IO PCI_BAR_SPACE_IO

#define PCI_BASE_ADDRESS_IO_MASK PCI_IOBASE_MASK

#define PCI_BASE_ADDRESS_MEM_MASK    PCI_MEMBASE_MASK
#define PCI_BASE_ADDRESS_MEM_TYPE_1M PCI_BAR_MEM_BELOW_1MB
#define PCI_BASE_ADDRESS_MEM_TYPE_64 PCI_BAR_MEM_ADDR64

#define PCI_ROM_ADDRESS      PCI_CFG_EXPANSION_ROM
#define PCI_ROM_ADDRESS_MASK (~0x7ffUL)
#define PCI_INTERRUPT_LINE   PCI_CFG_DEV_INT_LINE

#endif /* OSDPCI_H_INC */
