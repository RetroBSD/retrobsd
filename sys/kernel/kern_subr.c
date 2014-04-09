/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "systm.h"
#include "user.h"
#include "buf.h"
#include "uio.h"

/*
 * Move data to/from user space.
 */
int
uiomove (cp, n, uio)
	caddr_t cp;
	u_int n;
	register struct uio *uio;
{
	register struct iovec *iov;
	int error = 0;
	register u_int cnt;

	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;
		if (uio->uio_rw == UIO_READ)
			bcopy ((caddr_t) cp, iov->iov_base, cnt);
		else
			bcopy (iov->iov_base, (caddr_t) cp, cnt);
		iov->iov_base += cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp += cnt;
		n -= cnt;
	}
	return (error);
}

/*
 * Give next character to user as result of read.
 */
int
ureadc (c, uio)
	register int c;
	register struct uio *uio;
{
	register struct iovec *iov;

again:
	if (uio->uio_iovcnt == 0)
		panic("ureadc");
	iov = uio->uio_iov;
	if (iov->iov_len == 0 || uio->uio_resid == 0) {
		uio->uio_iovcnt--;
		uio->uio_iov++;
		goto again;
	}
	*iov->iov_base = c;

	iov->iov_base++;
	iov->iov_len--;
	uio->uio_resid--;
	uio->uio_offset++;
	return (0);
}

/*
 * Get next character written in by user from uio.
 */
int
uwritec(uio)
	register struct uio *uio;
{
	register struct iovec *iov;
	register int c;

	if (uio->uio_resid == 0)
		return (-1);
again:
	if (uio->uio_iovcnt <= 0)
		panic("uwritec");
	iov = uio->uio_iov;
	if (iov->iov_len == 0) {
		uio->uio_iov++;
		if (--uio->uio_iovcnt == 0)
			return (-1);
		goto again;
	}
	c = (u_char) *iov->iov_base;

	iov->iov_base++;
	iov->iov_len--;
	uio->uio_resid--;
	uio->uio_offset++;
	return (c & 0377);
}

/*
 * Copy bytes to/from the kernel and the user.
 */
int
uiofmove(cp, n, uio, iov)
	caddr_t cp;
	register int n;
	struct uio *uio;
	struct iovec *iov;
{
	if (uio->uio_rw == UIO_READ) {
		/* From kernel to user. */
		bcopy(cp, iov->iov_base, n);
	} else {
		/* From user to kernel. */
		bcopy(iov->iov_base, cp, n);
	}
	return(0);
}
