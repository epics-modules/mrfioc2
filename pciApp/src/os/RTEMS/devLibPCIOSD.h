#ifndef OSDPCI_H_INC
#define OSDPCI_H_INC

#include <rtems/pci.h>
#include <rtems/endian.h>
#include <rtems/irq.h>

/* 0 <= N <= 5 */
#define PCI_BASE_ADDRESS(N) ( PCI_BASE_ADDRESS_0 + 4*(N) )

#define UINT32 uint32_t

#endif /* OSDPCI_H_INC */
