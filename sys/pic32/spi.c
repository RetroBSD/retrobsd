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
#include "param.h"
#include "conf.h"
#include "user.h"
#include "ioctl.h"
#include "systm.h"
#include "uio.h"
#include "spi.h"
#include "spi_bus.h"

const struct devspec spidevs[] = {
    { 0, "spi1" },
    { 1, "spi2" },
    { 2, "spi3" },
    { 3, "spi4" },
    { 0, 0 }
};

#define NSPI    4       /* Ports SPI1...SPI4 */

/*
 * To enable debug output, uncomment the first line.
 */
//#define PRINTDBG printf
#ifndef PRINTDBG
#   define PRINTDBG(...) /*empty*/
#endif

int spi_fd[NSPI];

/*
 * Open /dev/spi# device.
 * Use default SPI parameters:
 * - rate 250 kHz;
 * - no sleect pin.
 */
int spidev_open (dev_t dev, int flag, int mode)
{
    int channel = minor (dev);

    if (channel >= NSPI)
        return ENXIO;

    if (u.u_uid != 0)
            return EPERM;

    spi_fd[channel] = spi_open(channel+1,NULL,NULL);
    if(spi_fd[channel]==-1)
        return ENODEV;
    return 0;
}

int spidev_close (dev_t dev, int flag, int mode)
{
    int channel = minor (dev);

    if (channel >= NSPI)
        return ENXIO;

    if (u.u_uid != 0)
            return EPERM;

    spi_close(spi_fd[channel]);
    return 0;
}

int spidev_read (dev_t dev, struct uio *uio, int flag)
{
    return 0;
}

int spidev_write (dev_t dev, struct uio *uio, int flag)
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
int spidev_ioctl (dev_t dev, u_int cmd, caddr_t addr, int flag)
{
    int channel = minor (dev);
    unsigned char *cval = (unsigned char *)addr;
    int nelem;
    static unsigned volatile *const tris[8] = {
        0, &TRISA,&TRISB,&TRISC,&TRISD,&TRISE,&TRISF,&TRISG,
    };
    int mask, portnum;

    //PRINTDBG ("spi%d: ioctl (cmd=%08x, addr=%08x)\n", channel+1, cmd, addr);
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
        if((unsigned int) addr & 0x01)
            spi_set(spi_fd[channel], PIC32_SPICON_CKE);
        if((unsigned int) addr & 0x02)
            spi_set(spi_fd[channel], PIC32_SPICON_CKP);
        return 0;

    case SPICTL_SETRATE:        /* set clock rate, kHz */
        spi_brg(spi_fd[channel], (unsigned int) addr);
        return 0;

    case SPICTL_SETSELPIN:      /* set select pin */
        mask = 1 << ((unsigned int) addr & 15);
        portnum = ((unsigned int) addr >> 8) & 7;
        if (! portnum)
            return 0;
        spi_set_cspin(spi_fd[channel], (unsigned int *)tris[((unsigned int) addr >> 8) & 7], (unsigned int) addr & 15);
        return 0;

    case SPICTL_IO8(0):         /* transfer n*8 bits */
        spi_select(spi_fd[channel]);
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (baduaddr (addr) || baduaddr (addr + nelem - 1))
            return EFAULT;
        spi_bulk_rw(spi_fd[channel], nelem, cval);
        spi_deselect(spi_fd[channel]);
        break;

    case SPICTL_IO16(0):        /* transfer n*16 bits */
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 1) ||
            baduaddr (addr) || baduaddr (addr + nelem*2 - 1))
            return EFAULT;
        spi_bulk_rw_16(spi_fd[channel], nelem<<1, (char *)addr);
        break;

    case SPICTL_IO32(0):        /* transfer n*32 bits */
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 3) ||
            baduaddr (addr) || baduaddr (addr + nelem*4 - 1))
            return EFAULT;
        spi_bulk_rw_32(spi_fd[channel], nelem<<2, (char *)addr);
        break;
// IM: added R and W and BE modes
    case SPICTL_IO8R(0):         /* transfer n*8 bits */
        spi_select(spi_fd[channel]);
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (baduaddr (addr) || baduaddr (addr + nelem - 1))
            return EFAULT;
        spi_bulk_read(spi_fd[channel], nelem, cval);
        spi_deselect(spi_fd[channel]);
        break;

    case SPICTL_IO16R(0):        /* transfer n*16 bits */
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 1) ||
            baduaddr (addr) || baduaddr (addr + nelem*2 - 1))
            return EFAULT;
        spi_bulk_read_16(spi_fd[channel], nelem<<1, (char *)addr);
        break;

    case SPICTL_IO32R(0):        /* transfer n*32 bits */
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 3) ||
            baduaddr (addr) || baduaddr (addr + nelem*4 - 1))
            return EFAULT;
        spi_bulk_read_32(spi_fd[channel], nelem<<2, (char *)addr);
        break;

    case SPICTL_IO8W(0):         /* transfer n*8 bits */
        spi_select(spi_fd[channel]);
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (baduaddr (addr) || baduaddr (addr + nelem - 1))
            return EFAULT;
        spi_bulk_write(spi_fd[channel], nelem, cval);
        spi_deselect(spi_fd[channel]);
        break;

    case SPICTL_IO16W(0):        /* transfer n*16 bits */
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 1) ||
            baduaddr (addr) || baduaddr (addr + nelem*2 - 1))
            return EFAULT;
        spi_bulk_write_16(spi_fd[channel], nelem<<1, (char *)addr);
        break;

    case SPICTL_IO32W(0):        /* transfer n*32 bits */
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 3) ||
            baduaddr (addr) || baduaddr (addr + nelem*4 - 1))
            return EFAULT;
        spi_bulk_write_32(spi_fd[channel], nelem<<2, (char *)addr);
        break;

    case SPICTL_IO32RB(0):        /* transfer n*32 bits */
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 3) ||
            baduaddr (addr) || baduaddr (addr + nelem*4 - 1))
            return EFAULT;
        spi_bulk_read_32_be(spi_fd[channel], nelem<<2, (char *)addr);
        break;

    case SPICTL_IO32WB(0):        /* transfer n*32 bits */
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 3) ||
            baduaddr (addr) || baduaddr (addr + nelem*4 - 1))
            return EFAULT;
        spi_bulk_write_32_be(spi_fd[channel], nelem<<2, (char *)addr);
        break;

    case SPICTL_IO32B(0):        /* transfer n*32 bits */
        nelem = (cmd >> 16) & IOCPARM_MASK;
        if (((unsigned) addr & 3) ||
            baduaddr (addr) || baduaddr (addr + nelem*4 - 1))
            return EFAULT;
        spi_bulk_write_32_be(spi_fd[channel], nelem<<2, (char *)addr);
        break;
//
    }
    return 0;
}
