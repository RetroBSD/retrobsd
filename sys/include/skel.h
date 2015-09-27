/*
 * Ioctl definitions for skeleton driver.
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
#ifndef _SKEL_H
#define _SKEL_H

#include <sys/ioctl.h>

#define SKELCTL_SETMODE     _IO ('?', 0)        /* set driver mode mode */

#define SKELCTL_IO(n)       _ION('?', 3, n)     /* transfer n bytes */

#ifdef KERNEL
#include "conf.h"

int skeldev_open (dev_t dev, int flag, int mode);
int skeldev_close (dev_t dev, int flag, int mode);
int skeldev_read (dev_t dev, struct uio *uio, int flag);
int skeldev_write (dev_t dev, struct uio *uio, int flag);
int skeldev_ioctl (dev_t dev, u_int cmd, caddr_t addr, int flag);
#endif

#endif
