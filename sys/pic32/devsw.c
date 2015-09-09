/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/inode.h>
#include <sys/proc.h>
#include <sys/clist.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/rdisk.h>
#include <sys/errno.h>
#include <sys/uart.h>
#include <sys/spi.h>

#include <sys/swap.h>

extern int strcmp(char *s1, char *s2);

#ifdef UARTUSB_ENABLED
#   include <sys/usb_uart.h>
#endif
#ifdef GPIO_ENABLED
#   include <sys/gpio.h>
#endif
#ifdef ADC_ENABLED
#   include <sys/adc.h>
#endif
#ifdef GLCD_ENABLED
#   include <sys/glcd.h>
#endif
#ifdef PWM_ENABLED
#   include <sys/pwm.h>
#endif
#ifdef PICGA_ENABLED
#   include <sys/picga.h>
#endif
#ifdef PTY_ENABLED
#   include <sys/pty.h>
#endif
#ifdef HX8357_ENABLED
#   include <sys/hx8357.h>
#endif
#ifdef SKEL_ENABLED
#   include <sys/skel.h>
#endif

/*
 * Null routine; placed in insignificant entries
 * in the bdevsw and cdevsw tables.
 */
int nulldev ()
{
    return (0);
}

int norw (dev, uio, flag)
    dev_t dev;
    struct uio *uio;
    int flag;
{
    return (0);
}

int noioctl (dev, cmd, data, flag)
    dev_t dev;
    u_int cmd;
    caddr_t data;
    int flag;
{
    return EIO;
}

/*
 * root attach routine
 */
void noroot (csr)
    caddr_t csr;
{
    /* Empty. */
}

/*
 * The RetroDisks require the same master number as the disk entry in the
 * rdisk.c file.  A bit of a bind, but it means that the RetroDisk
 * devices must be numbered from master 0 upwards.
 */
const struct bdevsw bdevsw[] = {
{   /* 0 - rd0 */
    rdopen,         rdclose,        rdstrategy,
    noroot,         rdsize,         rdioctl,        0, rd0devs
},
{   /* 1 - rd1 */
    rdopen,         rdclose,        rdstrategy,
    noroot,         rdsize,         rdioctl,        0, rd1devs
},
{   /* 2 - rd2 */
    rdopen,         rdclose,        rdstrategy,
    noroot,         rdsize,         rdioctl,        0, rd2devs
},
{   /* 3 - rd3 */
    rdopen,         rdclose,        rdstrategy,
    noroot,         rdsize,         rdioctl,        0, rd3devs
},
{   /* 4 - swap */
    swopen,         swclose,        swstrategy,
    noroot,         swsize,         swcioctl,       0, swapbdevs
},

{ 0 },
};

const int nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]) - 1;

const struct cdevsw cdevsw[] = {

/*
 * Static drivers - every system has these:
 */
{   /* 0 - console */
    cnopen,         cnclose,        cnread,         cnwrite,
    cnioctl,        nulldev,        cnttys,         cnselect,
    nostrategy,     0,              0,              cndevs
},
{   /* 1 - mem, kmem, null, zero */
    nulldev,        nulldev,        mmrw,           mmrw,
    noioctl,        nulldev,        0,              seltrue,
    nostrategy,     0,              0,              mmdevs
},
{   /* 2 - tty */
    syopen,         nulldev,        syread,         sywrite,
    syioctl,        nulldev,        0,              syselect,
    nostrategy,     0,              0,              sydevs
},
{   /* 3 - fd */
    fdopen,         nulldev,        norw,           norw,
    noioctl,        nulldev,        0,              seltrue,
    nostrategy,     0,              0,              fddevs
},
{   /* 4 - temp (temporary allocation in swap space) */
    swcopen,        swcclose,       swcread,        swcwrite,
    swcioctl,       nulldev,        0,              seltrue,
    nostrategy,     0,              0,              swapcdevs
},

/*
 * Optional drivers from here on:
 */
{   /* 5 - log */
#ifdef LOG_ENABLED
    logopen,        logclose,       logread,        norw,
    logioctl,       nulldev,        0,              logselect,
    nostrategy,     0,              0,              logdevs
#else
    0
#endif
},
{   /* 6 - tty uart */
#if defined(UART1_ENABLED) || defined(UART2_ENABLED) || \
    defined(UART3_ENABLED) || defined(UART4_ENABLED) || \
    defined(UART5_ENABLED) || defined(UART6_ENABLED)
    uartopen,       uartclose,      uartread,       uartwrite,
    uartioctl,      nulldev,        uartttys,       uartselect,
    nostrategy,     uartgetc,       uartputc,       uartdevs
#else
    0
#endif
},
{   /* 7 - tty usb */
#ifdef UARTUSB_ENABLED
    usbopen,        usbclose,       usbread,        usbwrite,
    usbioctl,       nulldev,        usbttys,        usbselect,
    nostrategy,     usbgetc,        usbputc,        usbdevs
#else
    0
#endif
},
{   /* 8, 9 - pty */
#ifdef PTY_ENABLED
    ptsopen,        ptsclose,       ptsread,        ptswrite,
    ptyioctl,       nulldev,        pt_tty,         ptcselect,
    nostrategy,     0,              0,              ptsdevs
}, {
    ptcopen,        ptcclose,       ptcread,        ptcwrite,
    ptyioctl,       nulldev,        pt_tty,         ptcselect,
    nostrategy,     0,              0,              ptcdevs
#else
    0 }, { 0
#endif
},
{   /* 10 - gpio */
#ifdef GPIO_ENABLED
    gpioopen,       gpioclose,      gpioread,       gpiowrite,
    gpioioctl,      nulldev,        0,              seltrue,
    nostrategy,     0,              0,              gpiodevs
#else
    0
#endif
},
{   /* 11 - adc */
#ifdef ADC_ENABLED
    adc_open,       adc_close,      adc_read,       adc_write,
    adc_ioctl,      nulldev,        0,              seltrue,
    nostrategy,     0,              0,              adcdevs
#else
    0
#endif
},
{   /* 12 - spi */
#if defined(SPI1_ENABLED) || defined(SPI2_ENABLED) || \
    defined(SPI3_ENABLED) || defined(SPI4_ENABLED)
    spidev_open,    spidev_close,   spidev_read,    spidev_write,
    spidev_ioctl,   nulldev,        0,              seltrue,
    nostrategy,     0,              0,              spidevs
#else
    0
#endif
},
{   /* 13 - glcd */
#ifdef GLCD_ENABLED
    glcd_open,      glcd_close,     glcd_read,      glcd_write,
    glcd_ioctl,     nulldev,        0,              seltrue,
    nostrategy,     0,              0,              glcddevs
#else
    0
#endif
},
{   /* 14 - pwm */
#ifdef PWM_ENABLED
    pwm_open,       pwm_close,      pwm_read,       pwm_write,
    pwm_ioctl,      nulldev,        0,              seltrue,
    nostrategy,     0,              0,              pwmdevs
#else
    0
#endif
},
{   /* 15 - picga */            // Ignore this for now - it's WIP.
#ifdef PICGA_ENABLED
    picga_open,     picga_close,    picga_read,     picga_write,
    picga_ioctl,    nulldev,        0,              seltrue,
    nostrategy,     0,              0,              picgadevs
#else
    0
#endif
},
{   /* 16 - tft */
#ifdef HX8357_ENABLED
    hx8357_open,    hx8357_close,   hx8357_read,    hx8357_write,
    hx8357_ioctl,   nulldev,        hx8357_ttys,    hx8357_select,
    nostrategy,     hx8357_getc,    hx8357_putc,    hx8357devs
#else
    0
#endif
},
{   /* 17 - skel */
#ifdef SKEL_ENABLED
    skeldev_open,   skeldev_close,  skeldev_read,   skeldev_write,
    skeldev_ioctl,  nulldev,        0,              seltrue,
    nostrategy,     0,              0,              skeldevs
#else
    0
#endif
},

/*
 * End the list with a blank entry
 */
{ 0 },
};

const int nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]) - 1;

/*
 * Routine that identifies /dev/mem and /dev/kmem.
 *
 * A minimal stub routine can always return 0.
 */
int
iskmemdev(dev)
    register dev_t dev;
{
    if (major(dev) == 1 && (minor(dev) == 0 || minor(dev) == 1))
        return (1);
    return (0);
}

/*
 * Routine to determine if a device is a disk.
 *
 * A minimal stub routine can always return 0.
 */
int
isdisk(dev, type)
    dev_t dev;
    register int type;
{
    if (type != IFBLK)
        return 0;

    switch (major(dev)) {
    case 0:                 /* rd0 */
    case 1:                 /* rd1 */
    case 2:                 /* rd2 */
    case 3:                 /* rd3 */
    case 4:                 /* sw */
        return (1);
    default:
        return (0);
    }
    /* NOTREACHED */
}

/*
 * Routine to convert from character to block device number.
 * A minimal stub routine can always return NODEV.
 */
int
chrtoblk(dev_t dev)
{
    return (NODEV);
}

/*
 * Search through the devspec entries in the cdevsw
 * table looking for a device name match.
 */
dev_t get_cdev_by_name(char *name)
{
    int maj, i;
    const struct devspec *devs;

    for (maj = 0; maj < nchrdev; maj++) {
        devs = cdevsw[maj].devs;
        if (! devs)
            continue;

        for (i = 0; devs[i].devname != 0; i++) {
            if (strcmp(devs[i].devname, name) == 0) {
                return makedev(maj, devs[i].unit);
            }
        }
    }
    return -1;
}

char *cdevname(dev_t dev)
{
    int maj = major(dev);
    const struct devspec *devs = cdevsw[maj].devs;
    int i;

    if (! devs)
        return 0;

    for (i=0; devs[i].devname != 0; i++) {
        if (devs[i].unit == minor(dev)) {
            return devs[i].devname;
        }
    }
    return 0;
}
