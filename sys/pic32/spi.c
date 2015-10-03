/*
 * Generic SPI driver for PIC32.
 *
 * Copyright (C) 2012 Serge Vakulenko, <serge@vak.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/kconfig.h>
#include <sys/spi.h>

#define NSPI    4       /* Ports SPI1...SPI4 */

/*
 * To enable debug output, uncomment the first line.
 */
//#define PRINTDBG printf
#ifndef PRINTDBG
#   define PRINTDBG(...) /*empty*/
#endif

struct spiio spi_io[NSPI];      /* Data for SPI driver */

/*
 * Open /dev/spi# device.
 * Use default SPI parameters:
 * - rate 250 kHz;
 * - no sleect pin.
 */
int spidev_open(dev_t dev, int flag, int mode)
{
    int channel = minor(dev);
    struct spiio *io = &spi_io[channel];

    if (channel >= NSPI)
        return ENXIO;

    if (u.u_uid != 0)
            return EPERM;

    if (! io->bus)
        return ENODEV;
    return 0;
}

int spidev_close(dev_t dev, int flag, int mode)
{
    int channel = minor(dev);

    if (channel >= NSPI)
        return ENXIO;

    if (u.u_uid != 0)
            return EPERM;

    return 0;
}

int spidev_read(dev_t dev, struct uio *uio, int flag)
{
    return 0;
}

int spidev_write(dev_t dev, struct uio *uio, int flag)
{
    return 0;
}

/*
 * SPI control operations:
 * - SPICTL_SETMODE   - set clock polarity and phase
 * - SPICTL_SETRATE   - set data rate in kHz
 * - SPICTL_SETSELPIN - set select pin
 * - SPICTL_IO8(n)    - n*8 bit RW transaction
 * - SPICTL_IO16(n)   - n*16 bit RW transaction
 * - SPICTL_IO32(n)   - n*32 bit RW transaction
 * - SPICTL_IO8R(n)    - n*8 bit R transaction
 * - SPICTL_IO16R(n)   - n*16 bit R transaction
 * - SPICTL_IO32R(n)   - n*32 bit R transaction
 * - SPICTL_IO8W(n)    - n*8 bit W transaction
 * - SPICTL_IO16W(n)   - n*16 bit W transaction
 * - SPICTL_IO32W(n)   - n*32 bit W transaction
 * - SPICTL_IO32RB(n)   - n*32 bit RB transaction (B - swaps byte's order)
 * - SPICTL_IO32WB(n)   - n*32 bit WB transaction
 * - SPICTL_IO32B(n)   - n*32 bit B transaction
 */
int spidev_ioctl(dev_t dev, u_int cmd, caddr_t addr, int flag)
{
    int channel = minor(dev);
    struct spiio *io = &spi_io[channel];
    unsigned char *cval = (unsigned char *)addr;
    int nelem;
    static unsigned volatile *const tris[8] = {
        0, &TRISA,&TRISB,&TRISC,&TRISD,&TRISE,&TRISF,&TRISG,
    };
    int portnum;

    //PRINTDBG("spi%d: ioctl(cmd=%08x, addr=%08x)\n", channel+1, cmd, addr);
    if (channel >= NSPI)
        return ENXIO;

    switch (cmd & ~(IOCPARM_MASK << 16)) {
    default:
        return ENODEV;

    case SPICTL_SETMODE:        /* set SPI mode */
        /*      --- Clock ----
         * Mode Polarity Phase
         *   0     0       0
         *   1     0       1
         *   2     1       0
         *   3     1       1
         */
        if ((unsigned) addr & 1)
            spi_set(io, PIC32_SPICON_CKE);
        if ((unsigned) addr & 2)
            spi_set(io, PIC32_SPICON_CKP);
        return 0;

    case SPICTL_SETRATE:        /* set clock rate, kHz */
        spi_brg(io, (unsigned int) addr);
        return 0;

    case SPICTL_SETSELPIN:      /* set select pin */
        portnum = ((unsigned int) addr >> 8) & 7;
        if (! portnum)
            return 0;
        spi_set_cspin(io, (unsigned*) tris[portnum], (unsigned) addr & 15);
        return 0;

    case SPICTL_IO8(0):         /* transfer n*8 bits */
        spi_select(io);
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (baduaddr(addr) || baduaddr(addr + nelem - 1))
            return EFAULT;
        spi_bulk_rw(io, nelem, cval);
        spi_deselect(io);
        break;

    case SPICTL_IO16(0):        /* transfer n*16 bits */
        spi_select(io);
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 1) ||
            baduaddr(addr) || baduaddr(addr + nelem*2 - 1))
            return EFAULT;
        spi_bulk_rw_16(io, nelem<<1, (char *)addr);
        spi_deselect(io);
        break;

    case SPICTL_IO32(0):        /* transfer n*32 bits */
        spi_select(io);
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 3) ||
            baduaddr(addr) || baduaddr(addr + nelem*4 - 1))
            return EFAULT;
        spi_bulk_rw_32(io, nelem<<2, (char *)addr);
        spi_deselect(io);
        break;

// IM: added R and W and BE modes
    case SPICTL_IO8R(0):         /* transfer n*8 bits */
        spi_select(io);
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (baduaddr(addr) || baduaddr(addr + nelem - 1))
            return EFAULT;
        spi_bulk_read(io, nelem, cval);
        spi_deselect(io);
        break;

    case SPICTL_IO16R(0):        /* transfer n*16 bits */
        spi_select(io);
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 1) ||
            baduaddr(addr) || baduaddr(addr + nelem*2 - 1))
            return EFAULT;
        spi_bulk_read_16(io, nelem<<1, (char *)addr);
        spi_deselect(io);
        break;

    case SPICTL_IO32R(0):        /* transfer n*32 bits */
        spi_select(io);
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 3) ||
            baduaddr(addr) || baduaddr(addr + nelem*4 - 1))
            return EFAULT;
        spi_bulk_read_32(io, nelem<<2, (char *)addr);
        spi_deselect(io);
        break;

    case SPICTL_IO8W(0):         /* transfer n*8 bits */
        spi_select(io);
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (baduaddr(addr) || baduaddr(addr + nelem - 1))
            return EFAULT;
        spi_bulk_write(io, nelem, cval);
        spi_deselect(io);
        break;

    case SPICTL_IO16W(0):        /* transfer n*16 bits */
        spi_select(io);
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 1) ||
            baduaddr(addr) || baduaddr(addr + nelem*2 - 1))
            return EFAULT;
        spi_bulk_write_16(io, nelem<<1, (char *)addr);
        spi_deselect(io);
        break;

    case SPICTL_IO32W(0):        /* transfer n*32 bits */
        spi_select(io);
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 3) ||
            baduaddr(addr) || baduaddr(addr + nelem*4 - 1))
            return EFAULT;
        spi_bulk_write_32(io, nelem<<2, (char *)addr);
        spi_deselect(io);
        break;

    case SPICTL_IO32RB(0):        /* transfer n*32 bits */
        spi_select(io);
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 3) ||
            baduaddr(addr) || baduaddr(addr + nelem*4 - 1))
            return EFAULT;
        spi_bulk_read_32_be(io, nelem<<2, (char *)addr);
        spi_deselect(io);
        break;

    case SPICTL_IO32WB(0):        /* transfer n*32 bits */
        spi_select(io);
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 3) ||
            baduaddr(addr) || baduaddr(addr + nelem*4 - 1))
            return EFAULT;
        spi_bulk_write_32_be(io, nelem<<2, (char *)addr);
        spi_deselect(io);
        break;

    case SPICTL_IO32B(0):        /* transfer n*32 bits */
        spi_select(io);
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 3) ||
            baduaddr(addr) || baduaddr(addr + nelem*4 - 1))
            return EFAULT;
        spi_bulk_write_32_be(io, nelem<<2, (char *)addr);
        spi_deselect(io);
        break;
    }
    return 0;
}

/*
 * Test to see if device is present.
 * Return true if found and initialized ok.
 * SPI ports are always present, if configured.
 */
static int
spiprobe(config)
    struct conf_ctlr *config;
{
    int channel = config->ctlr_unit - 1;
    struct spiio *io = &spi_io[channel];
    int sdi, sdo, sck;
    static const int sdi_tab[NSPI] = {
        GPIO_PIN('C',4),    /* SDI1 */
        GPIO_PIN('G',7),    /* SDI2 */
        GPIO_PIN('D',2),    /* SDI3: 64pin - RD2, 100pin - RF2 */
        GPIO_PIN('F',4),    /* SDI4 */
    };
    static const int sdo_tab[NSPI] = {
        GPIO_PIN('D',0),    /* SDO1 */
        GPIO_PIN('G',8),    /* SDO2 */
        GPIO_PIN('D',3),    /* SDO3: 64pin - RD3, 100pin - RF8 */
        GPIO_PIN('F',5),    /* SDO4 */
    };
    static const int sck_tab[NSPI] = {
        GPIO_PIN('D',10),   /* SCK1 */
        GPIO_PIN('G',6),    /* SCK2 */
        GPIO_PIN('D',1),    /* SCK3: 64pin - RD1, 100pin - RD15 */
        GPIO_PIN('D',10),   /* SCK4 */
    };

    if (channel < 0 || channel >= NSPI)
        return 0;
    sdi = sdi_tab[channel];
    sdo = sdo_tab[channel];
    sck = sck_tab[channel];
    if (channel+1 == 3 && cpu_pins > 64) {
        /* Port SPI3 has different pin assignment for 100-pin packages. */
        sdi = GPIO_PIN('F',2);
        sdo = GPIO_PIN('F',8);
        sck = GPIO_PIN('D',15);
    }
    printf("spi%u: pins sdi=R%c%d/sdo=R%c%d/sck=R%c%d\n", channel+1,
        gpio_portname(sdi), gpio_pinno(sdi),
        gpio_portname(sdo), gpio_pinno(sdo),
        gpio_portname(sck), gpio_pinno(sck));

    if (spi_setup(io, channel+1, 0) != 0) {
        printf("spi%u: setup failed\n", channel+1);
        return 0;
    }
    return 1;
}

struct driver spidriver = {
    "spi", spiprobe,
};
