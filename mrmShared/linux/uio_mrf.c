/*************************************************************************\
* Copyright (c) 2014 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include "mrf.h"

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

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
MODULE_PARM_DESC(interfaceversion, "User space interface version");

/************************ PCI Device and vendor IDs ****************/

#define PCI_SUBVENDOR_ID_MRF                0x1a3e

#define PCI_VENDOR_ID_LATTICE               0x1204

#define PCI_DEVICE_ID_EC_30                 0xEC30

#define PCI_DEVICE_ID_PLX_9030              0x9030      /** PCI Device ID for PLX-9030 bridge chip */
#define PCI_DEVICE_ID_PLX_9056              0x9056      /** PCI Device ID for PLX-9056 bridge chip */

/* PMC-EVR-230 */
#define PCI_SUBDEVICE_ID_MRF_PMCEVR_230     0x11e6
/* cPCI-EVR-230 */
#define PCI_SUBDEVICE_ID_MRF_PXIEVR_230     0x10e6
/* cPCI-EVG-230 */
#define PCI_SUBDEVICE_ID_MRF_PXIEVG_230     0x20E6
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)
/** Linux 3.12 adds a size test when mapping UIO_MEM_PHYS ranges
 *  to fix an clear security issue.  7314e613d5ff9f0934f7a0f74ed7973b903315d1
 *
 *  Unfortunately this makes it impossible to map ranges less than a page,
 *  such as the control registers for the PLX bridges (128 bytes).
 *  A further change in b65502879556d041b45104c6a35abbbba28c8f2d
 *  prevents mapping of ranges which don't start on a page boundary,
 *  which is also true of the PLX chips (offset 0xc00 on my test system).
 *
 *  This remains the case though the present (4.1 in May 2015).
 *
 *  These patches have been applied to the Debian 3.2.0 kernel.
 *
 *  The following is based uio_mmap_physical() from 3.2.0.
 */
static
int mrf_mmap_physical(struct uio_info *info, struct vm_area_struct *vma)
{
    struct pci_dev *dev = info->priv;
    int mi = vma->vm_pgoff; /* bounds check already done in uio_mmap() */

    if (vma->vm_end - vma->vm_start > PAGE_ALIGN(info->mem[mi].size)) {
        dev_err(&dev->dev, "mmap alignment/size test fails %lx %lx %u\n",
                vma->vm_start, vma->vm_end, (unsigned)PAGE_ALIGN(info->mem[mi].size));
        return -EINVAL;
    }

    vma->vm_flags |= VM_IO | VM_RESERVED;
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    return remap_pfn_range(vma,
                           vma->vm_start,
                           info->mem[mi].addr >> PAGE_SHIFT,
                           vma->vm_end - vma->vm_start,
                           vma->vm_page_prot);
}

#define USE_CUSTOM_MMAP
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
    case PCI_DEVICE_ID_EC_30: {
        static int flag = 0; /* don't spam! */
        if(!flag) {
            dev_err(&dev->dev, "EC30 ISR not supported in EVR mode");
            flag = 1;
        }
    }
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

    if (!(status & enable)) {
            return IRQ_NONE;
    }

    if(!(enable & IRQ_Enable)) {
        dev_info(&dev->dev, "Interrupt when not enabled! 0x%08lx 0x%08lx\n",
                 (unsigned long)enable, (unsigned long)status);
    }

    /* Disable interrupt */
    if (end) {
        iowrite32be(enable & ~IRQ_Enable, base + IRQEnable);
    } else {
        iowrite32(enable & ~IRQ_Enable, base + IRQEnable);
    }

    /* Ensure interrupts have really been disabled */
    wmb();
    if (end) {
        enable = ioread32be(base + IRQEnable);
    } else {
        enable = ioread32(base + IRQEnable);
    }

    if(enable & IRQ_Enable) {
        dev_info(&dev->dev, "Interrupt not disabled!!!");
    }

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
    void __iomem *plx = info->mem[0].internal_addr;
    u32 val, end;
    int oops;

    switch(dev->device) {
    case PCI_DEVICE_ID_PLX_9030:
        // clear INTCSR_PCI_Enable
        iowrite32(INTCSR_INT1_Enable|INTCSR_INT1_Polarity, plx+INTCSR);
        val = ioread32(plx+INTCSR);
        oops = val&INTCSR_PCI_Enable;
        break;

    case PCI_DEVICE_ID_PLX_9056:
        // clear INTCSR9056_PCI_Enable
        iowrite32(INTCSR9056_LCL_Enable, plx+INTCSR9056);
        val = ioread32(plx+INTCSR9056);
        oops = val&INTCSR9056_PCI_Enable;
        break;

    case PCI_DEVICE_ID_EC_30:
        // there are no distict registers for this bridge.
        // 'plx' holds the base address of FPGA registers

        // Check endianness
        val = ioread32(plx + FPGAVersion);
        if(((val & FPGAVer_FF) >> 24) == FPGAVER_EVR300) {
            // little endian
            end = 0;
        } else {
            val = ioread32be(plx + FPGAVersion);
            if(((val & FPGAVer_FF) >> 24) == FPGAVER_EVR300) {
                // big endian
                end = 1;
            } else {
                // This should never happen.
                dev_info(&dev->dev, "ERROR: Unable to read form factor magic number.");
                return IRQ_NONE;
            }
        }

        if(end) {
            val = ioread32be(plx + IRQEnable);
        } else {
            val = ioread32(plx + IRQEnable);
        }

        if(!(val & IRQ_Enable_ALL)) {
            dev_info(&dev->dev, "ERROR: Interrupt happening when not enabled!");
            return IRQ_NONE;
        }

        // Clear interrupts on FPGA
        if(end) {
            iowrite32be(val & (~IRQ_PCIee), plx + IRQEnable);
        } else {
            iowrite32(val & (~IRQ_PCIee), plx + IRQEnable);
        }

        // Check if clear succeded
        wmb();
        if(end) {
            val = ioread32be(plx + IRQEnable);
        } else {
            val = ioread32(plx + IRQEnable);
        }

        oops = val & IRQ_PCIee;
        break;

    default:
        return IRQ_NONE;
    }

    if(oops) {
        dev_info(&dev->dev, "Interrupt not disabled %08x", (unsigned)val);
    }

    return IRQ_HANDLED;
}

static
irqreturn_t
mrf_handler(int irq, struct uio_info *info)
{
    struct mrf_priv *priv = container_of(info, struct mrf_priv, uio);

    // Count interrupt handler executions
    priv->intrcount++;

    rmb();
    if(priv->irqmode) {
        return mrf_handler_plx(irq, info);
    } else {
        return mrf_handler_evr(irq, info);
    }
}

static
int mrf_irqcontrol(struct uio_info *info, s32 onoff)
{
    struct pci_dev *dev = info->priv;
    struct mrf_priv *priv = container_of(info, struct mrf_priv, uio);
    void __iomem *plx = info->mem[0].internal_addr;
    u32 val, end;

    if (onoff < 0) {
        return -EINVAL;
    } else if (onoff > 1) {
        return 0; /* onoff>1 is a no-op now, but may have other meaning in future */
    }

    switch (dev->device) {
    case PCI_DEVICE_ID_PLX_9030:

        val = INTCSR_INT1_Enable | INTCSR_INT1_Polarity;
        if (onoff == 1) {
            val |= INTCSR_PCI_Enable;
        }

        iowrite32(val, plx + INTCSR);

        priv->irqmode = 1;
        break;

    case PCI_DEVICE_ID_PLX_9056:

        val = INTCSR9056_LCL_Enable;
        if (onoff == 1) {
            val |= INTCSR9056_PCI_Enable;
        }

        iowrite32(val, plx + INTCSR9056);

        break;

    case PCI_DEVICE_ID_EC_30:
        // there are no distict registers for this bridge.
        // 'plx' holds the base address of FPGA registers
	
        // Check endianism
        val = ioread32(plx + FPGAVersion);
        if(((val & FPGAVer_FF) >> 24) == FPGAVER_EVR300) {
            end = 0;
        } else {
            val = ioread32be(plx + FPGAVersion);
            if(((val & FPGAVer_FF) >> 24) == FPGAVER_EVR300) {
                end = 1;
            } else {
                dev_info(&dev->dev, "ERROR: Unable to read form factor magic number.");
                return -EINVAL;
            }
        }

        // Read current IRQ enable register
        if(end) {
            val = ioread32be(plx + IRQEnable);
        } else {
            val = ioread32(plx + IRQEnable);
        }

        // Modify the IRQ enable bit
        if (onoff == 1) {
            val |= IRQ_PCIee;
        } else {
            val &= (~IRQ_PCIee);
        }

        // Write the register back
        if(end) {
            iowrite32be(val, plx + IRQEnable);
        } else {
            iowrite32(val, plx + IRQEnable);
        }

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
        if (!priv) { return -ENOMEM; }
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

        if (pci_request_regions(dev, DRV_NAME)) {
                goto err_disable;
        }

        /* PCIe EVR 300 has only 1 BAR so it must be treated differently.
         * EVR memory space is mapped directly to uio0 region so that it
         * matches windows version
         */
        if(dev->subsystem_device == PCI_SUBDEVICE_ID_PCIE_EVR_300){
            dev_info(&dev->dev, "Attaching BAR0 of PCIe-EVR-300\n");
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
#ifdef USE_CUSTOM_MMAP
        info->mmap = mrf_mmap_physical;
#endif

        info->name = DRV_NAME;
        info->version = DRV_VERSION;
        info->priv = dev;

        pci_set_drvdata(dev, info);

        ret = uio_register_device(&dev->dev, info);
        if (ret) {
            goto err_unmap;
        }

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
        .subdevice =    PCI_SUBDEVICE_ID_MRF_PXIEVG_230,
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
        .subdevice =    PCI_SUBDEVICE_ID_PCIE_EVR_300,
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
            if(dev->subsystem_device != PCI_SUBDEVICE_ID_PCIE_EVR_300) {
                void __iomem *plx = info->mem[0].internal_addr;
                u32 val = ioread32(plx + GPIOC);
                // Disable output drivers for TCLK, TMS, and TDI
                val &= ~GPIOC_pin_dir(0);
                val &= ~GPIOC_pin_dir(1);
                val &= ~GPIOC_pin_dir(3);
                iowrite32(val, plx + GPIOC);
            }
        }
#endif
        uio_unregister_device(info);
        pci_set_drvdata(dev, NULL);
        iounmap(info->mem[0].internal_addr);

        if(dev->subsystem_device != PCI_SUBDEVICE_ID_PCIE_EVR_300) {
            iounmap(info->mem[2].internal_addr);
        }

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

