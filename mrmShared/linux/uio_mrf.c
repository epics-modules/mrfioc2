/* Copyright (C) 2010 Brookhaven National Lab
 * All rights reserved.
 * See file LICENSE for terms.
 */

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

#define DRV_NAME "mrf-pci"
#define DRV_VERSION "0"

MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRV_VERSION);
MODULE_AUTHOR("Michael Davidsaver <mdavidsaver@bnl.gov>");

static int modparam_gpiobase = -1;
module_param_named(gpiobase, modparam_gpiobase, int, 0440);
MODULE_PARM_DESC(gpiobase, "The GPIO number base. -1 means dynamic, which is the default.");

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
/* 8 are supported, but only 4 are used */
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

/************************ Compatability ****************************/


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
static
void __iomem *pci_ioremap_bar(struct pci_dev* pdev,int bar)
{
        if(!(pci_resource_flags(pdev,bar) & IORESOURCE_MEM )){
                WARN_ON(1);
                return NULL;
        }
        return ioremap_nocache(pci_resource_start(pdev,bar),
                pci_resource_len(pdev,bar));
}
#endif

/**************************** PCI Driver *************************/


struct mrf_priv {
    struct uio_info uio;
#ifdef CONFIG_GENERIC_GPIO
    struct gpio_chip gpio;
    struct spinlock lock;
    int gpio_cleanup;
#endif
    struct pci_dev *pdev;
};

#ifdef CONFIG_GENERIC_GPIO
/************************* GPIO for JTAG **************************/

static const char * const GPIOC_names[GPIOC_num_pins] = {
    "TCK",
    "TMS",
    "TDO", /* output from device, input to computer */
    "TDI", /* input to device, output from computer */
};

static
int mrf_gpio_request(struct gpio_chip *chip, unsigned offset)
{
    u32 val;
    struct mrf_priv *priv = container_of(chip, struct mrf_priv, gpio);
    void __iomem *plx = priv->uio.mem[2].internal_addr;

    if(offset>=GPIOC_num_pins)
        return -EINVAL;
    if(offset==2)
        return 0;

    spin_lock(&priv->lock);

    val = ioread32(plx + GPIOC) & GPIOC_pin_dir(offset);
    iowrite32(val, plx + GPIOC);

    spin_unlock(&priv->lock);

    dev_info(&priv->pdev->dev, "GPIO%u active\n", offset);
    return 0;
}

static
void mrf_gpio_free(struct gpio_chip *chip, unsigned offset)
{
    u32 val;
    struct mrf_priv *priv = container_of(chip, struct mrf_priv, gpio);
    void __iomem *plx = priv->uio.mem[2].internal_addr;

    if(offset>=GPIOC_num_pins)
        return;

    spin_lock(&priv->lock);

    /* When not in use turn off driver */
    val = ioread32(plx + GPIOC) & ~GPIOC_pin_dir(offset);
    iowrite32(val, plx + GPIOC);

    spin_unlock(&priv->lock);

    dev_info(&priv->pdev->dev, "GPIO%u inactive\n", offset);
}

static
int mrf_gpio_get(struct gpio_chip *chip, unsigned offset)
{
    int ret;
    struct mrf_priv *priv = container_of(chip, struct mrf_priv, gpio);
    void __iomem *plx = priv->uio.mem[2].internal_addr;

    if(offset>=GPIOC_num_pins)
        return 0;

    ret = ioread32(plx + GPIOC) & GPIOC_pin_data(offset);

    return !!ret;
}

static
void mrf_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
    u32 val;
    struct mrf_priv *priv = container_of(chip, struct mrf_priv, gpio);
    void __iomem *plx = priv->uio.mem[2].internal_addr;

    if(offset==2 || offset>=GPIOC_num_pins)
        return;

    spin_lock(&priv->lock);

    val = ioread32(plx + GPIOC);
    if(value)
        val |= GPIOC_pin_data(offset);
    else
        val &= ~GPIOC_pin_data(offset);

    iowrite32(val, plx + GPIOC);

    spin_unlock(&priv->lock);
}

static
int mrf_gpio_dir_in(struct gpio_chip *chip, unsigned offset)
{
    if(offset!=2) /* Only TDO may be input */
        return -EINVAL;
    return 0;
}

static
int mrf_gpio_dir_out(struct gpio_chip *chip, unsigned offset, int value)
{
    if(offset==2) /* All but TDO are output */
        return -EINVAL;
    mrf_gpio_set(chip, offset, value);
    return 0;
}
#endif

/******************** PCI interrupt handler ***********************/

static
irqreturn_t
mrf_handler(int irq, struct uio_info *info)
{
    void __iomem *base = info->mem[2].internal_addr;
    void __iomem *plx = info->mem[0].internal_addr;
    struct pci_dev *dev = info->priv;
    u32 plxctrl, status, enable;
    int end;

    switch(dev->device) {
    case PCI_DEVICE_ID_PLX_9030:
        plxctrl = ioread32(plx + LAS0BRD);
        end = plxctrl & LAS0BRD_ENDIAN;
        break;
    case PCI_DEVICE_ID_PLX_9056:
        plxctrl = ioread32(plx + BIGEND9056);
        end = plxctrl & BIGEND9056_BIG;
        break;
    default:
        return IRQ_NONE; /* hope this never happens :) */
    }

    /* Test endianness since we allow user
     * apps to use whatever is convenient
     */
    if (end) {
        status = ioread32be(base + IRQFlag);
        enable = ioread32be(base + IRQEnable);
    } else {
        status = ioread32(base + IRQFlag);
        enable = ioread32(base + IRQEnable);
    }

    if (!(status & enable))
            return IRQ_NONE;

    if(!(enable & IRQ_Enable))
        dev_info(&dev->dev, "Interrupt when not enabled! 0x%08lx 0x%08lx\n",
                 (unsigned long)enable, (unsigned long)status);

    /* Disable interrupt */
    if (end)
        iowrite32be(enable & ~IRQ_Enable, base + IRQEnable);
    else
        iowrite32(enable & ~IRQ_Enable, base + IRQEnable);
    /* Ensure interrupts have really been disabled */
    wmb();
    if (end) {
        enable = ioread32be(base + IRQEnable);
    } else {
        enable = ioread32(base + IRQEnable);
    }
    if(enable & IRQ_Enable)
        dev_info(&dev->dev, "Interrupt not disabled!!!");


    return IRQ_HANDLED;
}

/************************* Initialization ***************************/

static
int __devinit
mrf_probe(struct pci_dev *dev,
          const struct pci_device_id *id)
{
        int ret = -ENODEV;
        struct mrf_priv *priv;
        struct uio_info *info;

        priv = kzalloc(sizeof(struct mrf_priv), GFP_KERNEL);
        if (!priv) return -ENOMEM;
        info = &priv->uio;
        priv->pdev = dev;

        ret = pci_enable_device(dev);
        if (ret) {
                dev_err(&dev->dev, "pci_enable_device failed with %d\n",ret);
                goto err_free;
        }
        if (!dev->irq) {
                dev_warn(&dev->dev, "Device not configured with IRQ!\n");
                ret=-ENODEV;
                goto err_disable;
        }

        if (pci_request_regions(dev, DRV_NAME))
                goto err_disable;

        /* BAR 0 is the PLX bridge */
        info->mem[0].addr = pci_resource_start(dev, 0);
        info->mem[0].size = pci_resource_len(dev,0);
        info->mem[0].internal_addr =pci_ioremap_bar(dev,0);
        info->mem[0].memtype = UIO_MEM_PHYS;

        /* Not used */
        info->mem[1].memtype = UIO_MEM_NONE;
        info->mem[1].size = 1; /* Otherwise UIO will stop searching... */

        /* BAR 2 is the EVR */
        info->mem[2].addr = pci_resource_start(dev, 2);
        info->mem[2].size = pci_resource_len(dev,2);
        info->mem[2].internal_addr =pci_ioremap_bar(dev,2);
        info->mem[2].memtype = UIO_MEM_PHYS;

        if (!info->mem[0].internal_addr ||
            !info->mem[0].addr ||
            !info->mem[2].internal_addr ||
            !info->mem[2].addr) {
                dev_err(&dev->dev, "Failed to map BARS!\n");
                ret=-ENODEV;
                goto err_release;
        }

        info->irq = dev->irq;
        info->irq_flags = IRQF_SHARED;
        info->handler = mrf_handler;

        info->name = DRV_NAME;
        info->version = DRV_VERSION;
        info->priv = dev;

        pci_set_drvdata(dev, info);

        ret = uio_register_device(&dev->dev, info);
        if (ret)
                goto err_unmap;

#ifdef CONFIG_GENERIC_GPIO
        spin_lock_init(&priv->lock);
        if (dev->device==PCI_DEVICE_ID_PLX_9030) {
            u32 val;
            void __iomem *plx = info->mem[2].internal_addr;

            /* GPIO Bits 0-3 are used as GPIO for JTAG.
             * Bits 4-7 must be left to their normal functions.
             * The device is expected to configure bits 4-7
             * itself when initialized, and not change them
             * afterward.  So we just avoid changes.
             *
             * Power up value observed in a PMC-EVR-230
             * GPIOC = 0x00249924
             * is consistent with the default given in the
             * PLX 9030 data book.
             */

            val = ioread32(plx + GPIOC);

            /* clear everything for GPIO 0-3 (aka first 12 bits).
             * Preserve current settings for GPIO 4-7.
             * This will setup these as inputs (which float high)
             */
            val &= 0xfffff000;

            iowrite32(val, plx + GPIOC);

            /* Setup GPIO config */
            priv->gpio.label = dev_name(&dev->dev);
            priv->gpio.owner = THIS_MODULE;
            priv->gpio.base = modparam_gpiobase;
            priv->gpio.ngpio = GPIOC_num_pins;
            priv->gpio.names = GPIOC_names;
            priv->gpio.can_sleep = 0;
            priv->gpio.request = &mrf_gpio_request;
            priv->gpio.free = &mrf_gpio_free;
            priv->gpio.direction_input = &mrf_gpio_dir_in;
            priv->gpio.direction_output = &mrf_gpio_dir_out;
            priv->gpio.set = &mrf_gpio_set;
            priv->gpio.get = &mrf_gpio_get;

            /* gpiochip register fail is not fatal.  Likely due
             * to collision of gpiobase numbers.  Must use dynamic
             * numbers when multiple MRF cards w/ PLX 9030 are present.
             */
            ret = gpiochip_add(&priv->gpio);
            if (ret) {
                dev_warn(&dev->dev, "GPIO setup error, JTAG unavailable\n");
            } else {
                priv->gpio_cleanup = 1;
                dev_info(&dev->dev, "GPIO setup ok, JTAG available at bit %d\n",
                         priv->gpio.base);
            }
        }
#else
        if (dev->device==PCI_DEVICE_ID_PLX_9030)
            dev_info(&dev->dev, "GPIO support not built, JTAG unavailable\n");
#endif

        dev_info(&dev->dev, "MRF Setup complete\n");

        return 0;
//err_unreg:
//        uio_unregister_device(info);
//        pci_set_drvdata(dev, NULL);
err_unmap:
        iounmap(info->mem[0].internal_addr);
        iounmap(info->mem[2].internal_addr);
err_release:
        pci_release_regions(dev);
err_disable:
        pci_disable_device(dev);
err_free:
        kfree(priv);
        return ret;
}

#define PCI_SUBVENDOR_ID_MRF             0x1a3e

/* PMC-EVR-230 */
#define PCI_SUBDEVICE_ID_MRF_PMCEVR_230   0x11e6
/* cPCI-EVR-230 */
#define PCI_SUBDEVICE_ID_MRF_PXIEVR_230   0x10e6
/* cPCI-EVRTG-300 */
#define PCI_SUBDEVICE_ID_MRF_EVRTG_300    0x192c

static struct pci_device_id mrf_pci_ids[] __devinitdata = {
    {
        .vendor =       PCI_VENDOR_ID_PLX,
        .device =       PCI_DEVICE_ID_PLX_9030,
        .subvendor =    PCI_SUBVENDOR_ID_MRF,
        .subdevice =    PCI_SUBDEVICE_ID_MRF_PXIEVR_230,
    },
    {
        .vendor =       PCI_VENDOR_ID_PLX,
        .device =       PCI_DEVICE_ID_PLX_9030,
        .subvendor =    PCI_SUBVENDOR_ID_MRF,
        .subdevice =    PCI_SUBDEVICE_ID_MRF_PMCEVR_230,
    },
    {
        .vendor =       PCI_VENDOR_ID_PLX,
        .device =       PCI_DEVICE_ID_PLX_9056,
        .subvendor =    PCI_SUBVENDOR_ID_MRF,
        .subdevice =    PCI_SUBDEVICE_ID_MRF_EVRTG_300,
    },
    { 0, }
};

/************************** Module boilerplate ****************************/

static
void __devexit
mrf_remove(struct pci_dev *dev)
{
        struct uio_info *info = pci_get_drvdata(dev);
        struct mrf_priv *priv = container_of(info, struct mrf_priv, uio);

#ifdef CONFIG_GENERIC_GPIO
        if (priv->gpio_cleanup) {
            int status = gpiochip_remove(&priv->gpio);
            while (status==-EBUSY) {
                msleep(10); /* operation in progress... zzzzz */
                status = gpiochip_remove(&priv->gpio);
            }
            BUG_ON(status && status!=-EBUSY);
            dev_info(&dev->dev, "GPIO Cleaned up\n");
        }
#endif
        uio_unregister_device(info);
        pci_set_drvdata(dev, NULL);
        iounmap(info->mem[0].internal_addr);
        iounmap(info->mem[2].internal_addr);
        pci_release_regions(dev);
        pci_disable_device(dev);

        kfree(priv);

        dev_info(&dev->dev, "MRF Cleaned up\n");
}


static struct pci_driver mrf_driver = {
    .name    =DRV_NAME,
    .id_table=mrf_pci_ids,
    .probe   = mrf_probe,
    .remove  = __devexit_p(mrf_remove),
};

static int __init mrf_init_module(void)
{
        return pci_register_driver(&mrf_driver);
}
module_init(mrf_init_module);

static void __exit mrf_exit_module(void)
{
        pci_unregister_driver(&mrf_driver);
}
module_exit(mrf_exit_module);
