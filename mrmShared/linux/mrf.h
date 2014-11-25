/* Copyright (C) 2014 Brookhaven National Lab
 * All rights reserved.
 * See file LICENSE for terms.
 */

#ifndef MRF_H
#define MRF_H

#include <linux/version.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uio_driver.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/delay.h>
#ifdef CONFIG_GENERIC_GPIO
#  include <linux/gpio.h>
#endif
#ifdef CONFIG_PARPORT_NOT_PC
#  include <linux/parport.h>
#endif


/************************ Register definitions ****************************/

/*
 * A selection of registers for the PLX PCI9030
 *
 * This device is exposed as BAR #0 on PCI and PMC
 * versions of the EVR
 */

/* Address space #0 is exposed as BAR #2 */
#define LAS0BRD  0x28
/* Set for big endian, clear for little endian (swapped) */
#  define LAS0BRD_ENDIAN 0x01000000

/* Interrupt control */
#define INTCSR   0x4c
#  define INTCSR_INT1_Enable   0x01
#  define INTCSR_INT1_Polarity 0x02
#  define INTCSR_INT1_Status   0x04
#  define INTCSR_INT2_Enable   0x08
#  define INTCSR_INT2_Polarity 0x10
#  define INTCSR_INT2_Status   0x20
#  define INTCSR_PCI_Enable    0x40
#  define INTCSR_SW_INTR       0x80

#define GPIOC   0x54
#  define GPIOC_pin0_fn   1
#  define GPIOC_pin0_dir  2
#  define GPIOC_pin0_data 4
#  define GPIOC_pin_fn(N)   (GPIOC_pin0_fn<<(3*(N)))
#  define GPIOC_pin_dir(N)  (GPIOC_pin0_dir<<(3*(N)))
#  define GPIOC_pin_data(N) (GPIOC_pin0_data<<(3*(N)))
/* 8 are supported, but only 4 are used.
 * 0 - TCLK, 1 - TMS, 2 - TDO, 3 - TDI
 */
#  define GPIOC_num_pins 4

/*
 * A selection of registers for the PLX PCI9056
 *
 * This device is exposed as BAR #0
 */

#define BIGEND9056 0x0C // 8 bit
#  define BIGEND9056_BIG (1<<2)

#define INTCSR9056 0x68 // 32 bit
#  define INTCSR9056_PCI_Enable (1<<8)

#  define INTCSR9056_DBL_Enable (1<<9)
#  define INTCSR9056_ABT_Enable (1<<10)
#  define INTCSR9056_LCL_Enable (1<<11)
#  define INTCSR9056_DBL_Status (1<<13) /* PCI doorbell */
#  define INTCSR9056_ABT_Status (1<<14) /* PCI abort */
#  define INTCSR9056_LCL_Status (1<<15) /* local */

#  define INTCSR9056_LBL_Enable (1<<17)
#  define INTCSR9056_DM0_Enable (1<<18)
#  define INTCSR9056_DM1_Enable (1<<19)
#  define INTCSR9056_LBL_Status (1<<20) /* Local doorbell */
#  define INTCSR9056_DM0_Status (1<<21) /* DMA 0 */
#  define INTCSR9056_DM1_Status (1<<22) /* DMA 1 */

#  define INTCSR9056_Status (INTCSR9056_DBL_Status| \
    INTCSR9056_ABT_Status|INTCSR9056_LCL_Status|INTCSR9056_LBL_Status| \
    INTCSR9056_DM0_Status|INTCSR9056_DM1_Status)

/* For MRM EVR 230 and 300 series
 */

#define IRQFlag     0x008
#  define IRQ_LinkChg   0x40
#  define IRQ_BufFull   0x20
#  define IRQ_HWMapped  0x10
#  define IRQ_Event     0x08
#  define IRQ_Heartbeat 0x04
#  define IRQ_FIFOFull  0x02
#  define IRQ_RXErr     0x01

#define IRQEnable   0x00c
/* Same bits as IRQFlag plus */
#  define IRQ_Enable    0x80000000

/* driver private struct */

struct mrf_priv {
    struct uio_info uio;
    struct pci_dev *pdev;
    unsigned int irqmode;

#if defined(CONFIG_GENERIC_GPIO) || defined(CONFIG_PARPORT_NOT_PC)
    spinlock_t lock;
#endif
#ifdef CONFIG_GENERIC_GPIO
    struct gpio_chip gpio;
    int gpio_cleanup;
#endif
#ifdef CONFIG_PARPORT_NOT_PC
    unsigned int ppenable;
    const struct ppcable *cable;
    struct parport *port;
#endif
};

#ifdef CONFIG_PARPORT_NOT_PC
void mrf_pp_setup(struct mrf_priv* dev);
void mrf_pp_cleanup(struct mrf_priv* priv);
#endif
#ifdef CONFIG_GENERIC_GPIO
void mrf_gpio_setup(struct mrf_priv *priv);
void mrf_gpio_cleanup(struct mrf_priv *priv);
#endif

#endif // MRF_H
