/*
 * Skeleton for a character device driver.
 *
 * Copyright (C) 2015 Serge Vakulenko
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
#include <sys/skel.h>

#define NSKEL   5       /* Ports 1...5 */

/*
 * To enable debug output, uncomment the first line.
 */
#define PRINTDBG printf
#ifndef PRINTDBG
#   define PRINTDBG(...) /*empty*/
#endif

/*
 * Open /dev/skel# device.
 */
int skeldev_open(dev_t dev, int flag, int mode)
{
    int unit = minor(dev);

    if (unit >= NSKEL)
        return ENXIO;

    if (u.u_uid != 0)
        return EPERM;

    // TODO: initialize the port.
    PRINTDBG("--- %s() unit=%u, flag=%d, mode=%d\n", __func__, unit, flag, mode);
    return 0;
}

/*
 * Close device.
 */
int skeldev_close(dev_t dev, int flag, int mode)
{
    int unit = minor(dev);

    if (u.u_uid != 0)
        return EPERM;

    // TODO: disable the port.
    PRINTDBG("--- %s() unit=%u, flag=%d, mode=%d\n", __func__, unit, flag, mode);
    return 0;
}

/*
 * Read data from device.
 */
int skeldev_read(dev_t dev, struct uio *uio, int flag)
{
    int unit = minor(dev);

    // TODO: read data from port to user program.
    PRINTDBG("--- %s() unit=%u, flag=%d\n", __func__, unit, flag);
    return 0;
}

/*
 * Write data to device.
 */
int skeldev_write(dev_t dev, struct uio *uio, int flag)
{
    int unit = minor(dev);

    // TODO: write data from user program to port.
    PRINTDBG("--- %s() unit=%u, flag=%d\n", __func__, unit, flag);
    return 0;
}

/*
 * Control operations:
 *      SKELCTL_SETMODE - set device mode
 *      SKELCTL_IO(n)   - perform R/W transaction of n bytes
 */
int skeldev_ioctl(dev_t dev, u_int cmd, caddr_t addr, int flag)
{
    int unit = minor(dev);
    int nbytes;

    PRINTDBG("--- %s() unit=%u, cmd=0x%08x, addr=0x%08x, flag=%d\n", __func__, unit, cmd, addr, flag);
    switch (cmd & ~(IOCPARM_MASK << 16)) {
    default:
        return ENODEV;

    case SKELCTL_SETMODE:
        // TODO: set device mode.
        PRINTDBG("--- SETMODE 0x%x\n", (unsigned) addr);
        return 0;

    case SKELCTL_IO(0):         /* transfer n bytes */
        nbytes = (cmd >> 16) & IOCPARM_MASK;
        if (baduaddr(addr) || baduaddr(addr + nbytes - 1))
            return EFAULT;

        // TODO: transfer nbytes from device to addr.
        PRINTDBG("--- Transfer nbytes=%u to addr=0x%08x\n", addr);
        break;
    }
    return 0;
}
