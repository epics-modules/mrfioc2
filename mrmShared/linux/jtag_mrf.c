/* Copyright (C) 2014 Brookhaven National Lab
 * All rights reserved.
 * See file LICENSE for terms.
 */
/* This file contains hooks to access
 * the GPIO lines of the PLX PCI9030
 * which on the PMC-EVR-230
 * are connected use for a JTAG interface.
 *
 * These lines may be accessed through
 * the GPIO sub-system, or through
 * an emulated parallel port programing cable.
 */

#include "mrf.h"

#ifdef CONFIG_GENERIC_GPIO
static int modparam_gpiobase = -1;
module_param_named(gpiobase, modparam_gpiobase, int, 0440);
MODULE_PARM_DESC(gpiobase, "The GPIO number base. -1 means dynamic, which is the default.");
#endif


#ifdef CONFIG_PARPORT_NOT_PC
// parallel port programming cable description
struct ppcable {
    const char *name;
    // bit masks for each JTAG signal

    // output (host -> devices signals)
    //  may be in data or control byte
    unsigned char tms_data, tms_ctrl;
    unsigned char tck_data, tck_ctrl;
    unsigned char tdi_data, tdi_ctrl;

    // input (device -> host)
    // may be in status or control byte
    unsigned char tdo_sts, tdo_ctrl;
};

static const struct ppcable cables[];
static char *cablename = "Minimal";
module_param_named(cable, cablename, charp, 0444);
MODULE_PARM_DESC(cable, "Name of JTAG parallel port cable to emulate");
#endif

#ifdef CONFIG_GENERIC_GPIO
/************************* GPIO for JTAG **************************/

static
int mrf_gpio_request(struct gpio_chip *chip, unsigned offset)
{
    if(offset>=GPIOC_num_pins)
        return -EINVAL;
    return 0;
}

static
void mrf_gpio_free(struct gpio_chip *chip, unsigned offset)
{}

static
int mrf_gpio_get(struct gpio_chip *chip, unsigned offset)
{
    int ret;
    struct mrf_priv *priv = container_of(chip, struct mrf_priv, gpio);
    void __iomem *plx = priv->uio.mem[0].internal_addr;

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
    void __iomem *plx = priv->uio.mem[0].internal_addr;

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

void mrf_gpio_setup(struct mrf_priv *priv)
{
    int ret;
    /* Setup GPIO config */
    priv->gpio.label = dev_name(&priv->pdev->dev);
    priv->gpio.owner = THIS_MODULE;
    priv->gpio.base = modparam_gpiobase;
    priv->gpio.ngpio = GPIOC_num_pins;
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
        dev_warn(&priv->pdev->dev, "GPIO setup error, JTAG unavailable\n");
    } else {
        priv->gpio_cleanup = 1;
        dev_info(&priv->pdev->dev, "GPIO setup ok, JTAG available at bit %d\n",
                 priv->gpio.base);
    }
}

void mrf_gpio_cleanup(struct mrf_priv *priv)
{
    if (priv->gpio_cleanup) {
        int status = gpiochip_remove(&priv->gpio);
        while (status==-EBUSY) {
            msleep(10); /* operation in progress... zzzzz */
            status = gpiochip_remove(&priv->gpio);
        }
        BUG_ON(status && status!=-EBUSY);
        dev_info(&priv->pdev->dev, "GPIO Cleaned up\n");
    }
}
#endif

/******************** Parallel port for JTAG **********************/
#ifdef CONFIG_PARPORT_NOT_PC

/* if this signal SIG is included in register REG (non-zero mask)
 * then set/clear the appropriate HW bit.
 * Translation between cable bit mask and GPIO bit mask
 */
#define PPBITSET(REG, SIG, BIT) \
  if(dev->cable->SIG ## _ ## REG)  {if(d & dev->cable->SIG ## _ ## REG) \
    {val |= GPIOC_pin_data(BIT);} else {val &= ~GPIOC_pin_data(BIT);}}

/* if this signal SIG is included in register REG (non-zero mask)
 * and the GPIO bit is active, then set the appropriate cable bit
 */
#define PPBITGET(REG, SIG, BIT) \
  if(dev->cable->SIG ## _ ## REG && val & GPIOC_pin_data(BIT)) \
  {ret |= dev->cable->SIG ## _ ## REG;}

static void mrfpp_write_data(struct parport *port, unsigned char d)
{
    struct mrf_priv *dev = port->private_data;
    void __iomem *plx = dev->uio.mem[0].internal_addr;
    u32 val;

    spin_lock(&dev->lock);
    if(!dev->ppenable) {
        spin_unlock(&dev->lock);
        return;
    }
    val = ioread32(plx + GPIOC);

    PPBITSET(data, tck, 0)
    PPBITSET(data, tms, 1)
    PPBITSET(data, tdi, 3)
    // Don't write TDO

    iowrite32(val, plx + GPIOC);
    spin_unlock(&dev->lock);
}

static void mrfpp_write_control(struct parport *port, unsigned char d)
{
    struct mrf_priv *dev = port->private_data;
    void __iomem *plx = dev->uio.mem[0].internal_addr;
    u32 val;

    d ^= 0x0b; // bits are hardware inverted

    spin_lock(&dev->lock);
    if(!dev->ppenable) {
        spin_unlock(&dev->lock);
        return;
    }
    val = ioread32(plx + GPIOC);

    PPBITSET(ctrl, tck, 0)
    PPBITSET(ctrl, tms, 1)
    PPBITSET(ctrl, tdi, 3)
    // Don't write TDO

    iowrite32(val, plx + GPIOC);
    spin_unlock(&dev->lock);
}

static unsigned char mrfpp_read_data(struct parport *port)
{
    unsigned char ret = 0;
    struct mrf_priv *dev = port->private_data;
    void __iomem *plx = dev->uio.mem[0].internal_addr;
    u32 val;

    val = ioread32(plx + GPIOC);
    PPBITGET(data, tck, 0)
    PPBITGET(data, tms, 1)
    PPBITGET(data, tdi, 3)

    return ret;
}

static unsigned char mrfpp_read_control(struct parport *port)
{
    unsigned char ret = 0;
    struct mrf_priv *dev = port->private_data;
    void __iomem *plx = dev->uio.mem[0].internal_addr;
    u32 val;

    val = ioread32(plx + GPIOC);
    PPBITGET(ctrl, tck, 0)
    PPBITGET(ctrl, tms, 1)
    PPBITGET(ctrl, tdo, 2)
    PPBITGET(ctrl, tdi, 3)
    ret ^= 0x0b; // bits are hardware inverted

    return ret;
}

static unsigned char mrfpp_read_status(struct parport *port)
{
    unsigned char ret = 0;
    struct mrf_priv *dev = port->private_data;
    void __iomem *plx = dev->uio.mem[0].internal_addr;
    u32 val;

    val = ioread32(plx + GPIOC);
    PPBITGET(sts, tdo, 2)
    ret ^= 0x80; // BUSY line is hardware inverted

    return ret;
}

static unsigned char mrfpp_frob_control(struct parport *dev, unsigned char mask,
                                        unsigned char val)
{
    unsigned char ret;

    // TODO, not atomic
    ret = mrfpp_read_control(dev);
    ret = (ret & ~mask) ^ val;
    mrfpp_write_control(dev, ret);
    return ret;
}

static void mrfpp_noop(struct parport *dev) {}

static void mrfpp_initstate(struct pardevice *dev, struct parport_state *s)
{
    // New clients clear all JTAG bits
    s->u.pc.ctr = 0x0b;
    s->u.pc.ecr = 0;
}

static void mrfpp_savestate(struct parport *dev, struct parport_state *s)
{
    s->u.pc.ctr = mrfpp_read_control(dev);
    s->u.pc.ecr = mrfpp_read_data(dev);
}
static void mrfpp_restorestate(struct parport *dev, struct parport_state *s)
{
    mrfpp_write_control(dev, s->u.pc.ctr);
    mrfpp_write_data(dev, s->u.pc.ecr);
}

static struct parport_operations mrfppops = {
    .owner = THIS_MODULE,

    .write_data    = mrfpp_write_data,
    .write_control = mrfpp_write_control,

    .read_data     = mrfpp_read_data,
    .read_status   = mrfpp_read_status,
    .read_control  = mrfpp_read_control,

    .frob_control  = mrfpp_frob_control,

    .enable_irq    = mrfpp_noop,
    .disable_irq   = mrfpp_noop,
    .data_forward  = mrfpp_noop,
    .data_reverse  = mrfpp_noop,

    .init_state    = mrfpp_initstate,
    .save_state    = mrfpp_savestate,
    .restore_state = mrfpp_restorestate,
};

void mrf_pp_setup(struct mrf_priv* dev)
{
    const struct ppcable *ccable = cables;

    for(; ccable->name; ccable++) {
        if(strcmp(ccable->name, cablename)==0)
            break;
    }
    if(!ccable) {
        dev_err(&dev->pdev->dev, "Cable '%s' is not defined\n", cablename);
        return;
    }

    dev->port = parport_register_port(0, PARPORT_IRQ_NONE,
                                      PARPORT_DMA_NONE,
                                      &mrfppops);

    if(!dev->port) {
        dev_err(&dev->pdev->dev, "Failed to register parallel port\n");
        return;
    }

    dev->cable = ccable;
    dev->port->modes = PARPORT_MODE_PCSPP; /* support only basic PC port ops */
    dev->port->private_data = dev;
    dev->port->dev = &dev->pdev->dev;

    parport_announce_port(dev->port);
    /* Protection from whatever random drivers have just tried
     * to print to us...
     */
    dev->ppenable = 1;

    dev_info(&dev->pdev->dev, "Emulating cable: %s\n", ccable->name);
    return;
}

void mrf_pp_cleanup(struct mrf_priv* priv)
{
    if(priv->port)
        parport_remove_port(priv->port);
}

#undef PPBITGET
#undef PPBITSET


static const struct ppcable cables[] =
{
  { // Urjtag minimal
    .name = "Minimal",
    .tms_data = 1 << 1,
    .tck_data = 1 << 2,
    .tdi_data = 1 << 3,
    .tdo_sts  = 1 << 7,
  },
  { // Altera ByteBlaster
    .name = "ByteBlaster",
    .tck_data = 1 << 0,
    .tms_data = 1 << 1,
    .tdi_data = 1 << 6,
    .tdo_sts  = 1 << 7,
  },
  {NULL}
};

#endif
