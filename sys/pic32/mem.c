/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "user.h"
#include "conf.h"
#include "systm.h"
#include "uio.h"

const struct devspec mmdevs[] = {
    { 0, "mem" },
    { 1, "kmem" },
    { 2, "null" },
    { 3, "zero" },
    { 0, 0 }
};

void kmemdev()
{
    u.u_rval = get_cdev_by_name("kmem");
}

/*
 * Read/write routine for /dev/mem family.
 */
int
mmrw (dev, uio, flag)
	dev_t dev;
	register struct uio *uio;
	int flag;
{
	register struct iovec *iov;
	int error = 0;
	register u_int c;
	caddr_t addr;

        //printf ("mmrw (dev=%u, len=%u, flag=%d)\n", dev, uio->uio_iov->iov_len, flag);
	while (uio->uio_resid && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("mmrw");
			continue;
		}
		switch (minor(dev)) {
		case 0:		/* minor device 0 is physical memory (/dev/mem) */
		case 1:		/* minor device 1 is kernel memory (/dev/kmem) */
		        addr = (caddr_t) uio->uio_offset;
                        if ((badkaddr (addr) && baduaddr (addr)) ||
                            (badkaddr (addr + iov->iov_len - 1) &&
                            baduaddr (addr + iov->iov_len - 1))) {
                                error = EFAULT;
                                break;
                        }
			error = uiomove(addr, iov->iov_len, uio);
			break;
		case 2:		/* minor device 2 is EOF/RATHOLE (/dev/null) */
			if (uio->uio_rw == UIO_READ)
				return(0);
			c = iov->iov_len;
			iov->iov_base += c;
			iov->iov_len -= c;
			uio->uio_offset += c;
			uio->uio_resid -= c;
			break;
		case 3:		/* minor device 3 is ZERO (/dev/zero) */
			if (uio->uio_rw == UIO_WRITE)
				return(EIO);
                        c = iov->iov_len;
                        bzero (iov->iov_base, c);
                        iov->iov_base += c;
                        iov->iov_len -= c;
                        uio->uio_offset += c;
                        uio->uio_resid -= c;
			break;
		default:
			return(EINVAL);
		}
	}
	return(error);
}
