/* Copyright (C) 2014 Brookhaven National Lab
 * All rights reserved.
 * See file LICENSE for terms.
 */

#include "mrf.h"

#define DRV_NAME "mrf-pci"
#define DRV_VERSION "1"

MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRV_VERSION);
MODULE_AUTHOR("Michael Davidsaver <mdavidsaver@bnl.gov>");

/* something that userspace can test to see if we
 * are the kernel module its looking for.
 */
static int modparam_iversion = 1;
module_param_named(interfaceversion, modparam_iversion, int, 0444);
MODULE_PARM_DESC(gpiobase, "User space interface version");

/************************ PCI Device and vendor IDs ****************/

#define PCI_SUBVENDOR_ID_MRF                0x1a3e

#define PCI_VENDOR_ID_LATTICE               0x1204

#define PCI_DEVICE_ID_EC_30                 0xEC30

/* PMC-EVR-230 */
#define PCI_SUBDEVICE_ID_MRF_PMCEVR_230     0x11e6
/* cPCI-EVR-230 */
#define PCI_SUBDEVICE_ID_MRF_PXIEVR_230     0x10e6
/* cPCI-EVRTG-300 */
#define PCI_SUBDEVICE_ID_MRF_EVRTG_300      0x192c
/* PCIe-EVR-300 */
#define PCI_SUBDEVICE_ID_PCIE_EVR_300       0x172c

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

/******************** PCI interrupt handler ***********************/

/* original ISR behavior which manipulates the EVR's
 * IRQFlag and IRQEnable.
 */
static
irqreturn_t
mrf_handler_evr(int irq, struct uio_info *info)
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

/* new ISR behavior which manipulates
 * the PLX bridge PCI interrupt enable bit.
 */
static
irqreturn_t
mrf_handler_plx(int irq, struct uio_info *info)
{
    struct pci_dev *dev = info->priv;
    void __iomem *plx;
    u32 val;
    int oops;

    switch(dev->device) {
    case PCI_DEVICE_ID_PLX_9030:
        plx = info->mem[0].internal_addr;
        // clear INTCSR_PCI_Enable
        iowrite32(INTCSR_INT1_Enable|INTCSR_INT1_Polarity, plx+INTCSR);
        val = ioread32(plx+INTCSR);
        oops = val&INTCSR_PCI_Enable;
        break;

    case PCI_DEVICE_ID_PLX_9056:
        plx = info->mem[0].internal_addr;
        // clear INTCSR9056_PCI_Enable
        iowrite32(INTCSR9056_LCL_Enable, plx+INTCSR9056);
        val = ioread32(plx+INTCSR9056);
        oops = val&INTCSR9056_PCI_Enable;
        break;

    default:
        return IRQ_NONE;
    }

    if(oops)
        dev_info(&dev->dev, "Interrupt not disabled %08x", (unsigned)val);

    return IRQ_HANDLED;
}



static
irqreturn_t
mrf_handler(int irq, struct uio_info *info)
{
    struct mrf_priv *priv = container_of(info, struct mrf_priv, uio);

    rmb();
    if(priv->irqmode)
        return mrf_handler_plx(irq, info);
    else
        return mrf_handler_evr(irq, info);
}

static
int mrf_irqcontrol(struct uio_info *info, s32 onoff)
{
    struct pci_dev *dev = info->priv;
    struct mrf_priv *priv = container_of(info, struct mrf_priv, uio);
    void __iomem *plx;
    u32 val;

    if (onoff < 0)
        return -EINVAL;
    else if (onoff > 1)
        return 0;

    switch (dev->device) {
    case PCI_DEVICE_ID_PLX_9030:
        plx = info->mem[0].internal_addr;

        val = INTCSR_INT1_Enable | INTCSR_INT1_Polarity;
        if (onoff == 1)
            val |= INTCSR_PCI_Enable;

        iowrite32(val, plx + INTCSR);

        priv->irqmode = 1;
        break;

    case PCI_DEVICE_ID_PLX_9056:
        plx = info->mem[0].internal_addr;

        val = INTCSR9056_LCL_Enable;
        if (onoff == 1)
            val |= INTCSR9056_PCI_Enable;

        iowrite32(val, plx + INTCSR9056);

        break;

    case PCI_DEVICE_ID_EC_30:
        plx = info->mem[0].internal_addr;

        // Check endianism

        // Read current IRQ enable register

        // Modify the IRQ enable bit

        // Write the register back

        val = INTCSR9056_LCL_Enable;
        if (onoff == 1)
            val |= INTCSR9056_PCI_Enable;

        iowrite32(val, plx + INTCSR9056);

        break;

    default:
        return -EINVAL;
    }

    // Writing 0 or 1 to /dev/uioX selects switches
    // interrupt handling to PLX mode
    priv->irqmode = 1;
    wmb();

    return 0;
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

        /* PCIe EVR 300 has only 1 BAR so it must be treated differently.
         * EVR memory space is mapped directly to uio0 region so that it
         * matches windows version
         */
        if(dev->subsystem_device == PCI_SUBDEVICE_ID_MRF_EVRTG_300E){
            dev_info(&dev->dev, "Attaching BAR0 of PCIe-EVRTG300e\n");
            /* BAR 0 is the EVR */
            info->mem[0].name = "EVR memory";
            info->mem[0].addr = pci_resource_start(dev, 0);
            info->mem[0].size = pci_resource_len(dev,0);
            info->mem[0].internal_addr = pci_ioremap_bar(dev,0);
            info->mem[0].memtype = UIO_MEM_PHYS;

            if(!info->mem[0].internal_addr || !info->mem[0].addr){
                dev_err(&dev->dev, "Failed to map BARS!\n");
                ret=-ENODEV;
                goto err_release;
            }

        }
        /* Other devices also have BAR 1 and 2 present (for PLX bridge).. */
        else{
            dev_info(&dev->dev, "Attaching BAR0,2,3 of MRF");

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
        }

        info->irq = dev->irq;
        info->irq_flags = IRQF_SHARED;
        info->handler = mrf_handler;
        info->irqcontrol = mrf_irqcontrol;

        info->name = DRV_NAME;
        info->version = DRV_VERSION;
        info->priv = dev;

        pci_set_drvdata(dev, info);

        ret = uio_register_device(&dev->dev, info);
        if (ret)
                goto err_unmap;

#if defined(CONFIG_GENERIC_GPIO) || defined(CONFIG_PARPORT_NOT_PC)
        spin_lock_init(&priv->lock);
        if (dev->device==PCI_DEVICE_ID_PLX_9030) {
            u32 val;
            void __iomem *plx = info->mem[0].internal_addr;

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
             * 
             * Each GPIO bit has 3 register bits (function, direction, and value)
             */
            val &= 0xfffff000;

            // Enable output drivers for TCLK, TMS, and TDI
            val |= GPIOC_pin_dir(0);
            val |= GPIOC_pin_dir(1);
            val |= GPIOC_pin_dir(3);

            dev_info(&dev->dev, "GPIOC %08x\n", val);
            iowrite32(val, plx + GPIOC);

#ifdef CONFIG_GENERIC_GPIO
            mrf_gpio_setup(priv);
#endif
#ifdef CONFIG_PARPORT_NOT_PC
            mrf_pp_setup(priv);
#endif
        }
#else
        if (dev->device==PCI_DEVICE_ID_PLX_9030) {
            dev_info(&dev->dev, "GPIO support not built, JTAG unavailable\n");
        }
#endif /* defined(CONFIG_GENERIC_GPIO) || defined(CONFIG_PARPORT_NOT_PC) */

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
        kzfree(priv);
        return ret;
}

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
    {
        .vendor =       PCI_VENDOR_ID_LATTICE,
        .device =       PCI_DEVICE_ID_EC_30,
        .subvendor =    PCI_SUBVENDOR_ID_MRF,
        .subdevice =    PCI_SUBDEVICE_ID_MRF_EVRTG_300E,
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

#ifdef CONFIG_PARPORT_NOT_PC
        mrf_pp_cleanup(priv);
#endif
#ifdef CONFIG_GENERIC_GPIO
        mrf_gpio_cleanup(priv);
#endif
#if defined(CONFIG_GENERIC_GPIO) || defined(CONFIG_PARPORT_NOT_PC)
        {
            void __iomem *plx = info->mem[0].internal_addr;
            u32 val = ioread32(plx + GPIOC);
            // Disable output drivers for TCLK, TMS, and TDI
            val &= ~GPIOC_pin_dir(0);
            val &= ~GPIOC_pin_dir(1);
            val &= ~GPIOC_pin_dir(3);
            iowrite32(val, plx + GPIOC);
        }
#endif
        uio_unregister_device(info);
        pci_set_drvdata(dev, NULL);
        iounmap(info->mem[0].internal_addr);
        iounmap(info->mem[2].internal_addr);
        pci_release_regions(dev);
        pci_disable_device(dev);

        kzfree(priv);

        dev_info(&dev->dev, "MRF Cleaned up\n");
}


static struct pci_driver mrf_driver = {
    .name     = DRV_NAME,
    .id_table = mrf_pci_ids,
    .probe    = mrf_probe,
    .remove   = __devexit_p(mrf_remove),
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

