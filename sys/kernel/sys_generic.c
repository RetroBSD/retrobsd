/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "user.h"
#include "proc.h"
#include "signalvar.h"
#include "inode.h"
#include "file.h"
#include "ioctl.h"
#include "conf.h"
#include "uio.h"
#include "kernel.h"
#include "systm.h"

int	selwait;

static void
rwuio (uio)
	register struct uio *uio;
{
	struct a {
		int	fdes;
	};
	register struct file *fp;
	register struct iovec *iov;
	u_int i, count;
	off_t	total;

	GETF(fp, ((struct a *)u.u_arg)->fdes);
	if ((fp->f_flag & (uio->uio_rw == UIO_READ ? FREAD : FWRITE)) == 0) {
		u.u_error = EBADF;
		return;
	}
	total = 0;
	uio->uio_resid = 0;
	for (iov = uio->uio_iov, i = 0; i < uio->uio_iovcnt; i++, iov++)
		total += iov->iov_len;

	uio->uio_resid = total;
	if (uio->uio_resid != total) {		/* check wraparound */
		u.u_error = EINVAL;
		return;
	}
	count = uio->uio_resid;
	if (setjmp (&u.u_qsave)) {
		/*
		 * The ONLY way we can get here is via the longjump in sleep.  Thus signals
		 * have been checked and u_error set accordingly.  If no bytes have been
		 * transferred then all that needs to be done now is 'return'; the system
		 * call will either be restarted or reported as interrupted.  If bytes have
		 * been transferred then we need to calculate the number of bytes transferred.
		 */
		if (uio->uio_resid == count)
			return;
		u.u_error = 0;
	} else
		u.u_error = (*Fops[fp->f_type]->fo_rw) (fp, uio);

	u.u_rval = count - uio->uio_resid;
}

/*
 * Read system call.
 */
void
read()
{
	register struct a {
		int	fdes;
		char	*cbuf;
		unsigned count;
	} *uap = (struct a *)u.u_arg;
	struct uio auio;
	struct iovec aiov;

	aiov.iov_base = (caddr_t)uap->cbuf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	rwuio (&auio);
}

void
readv()
{
	register struct a {
		int	fdes;
		struct	iovec *iovp;
		unsigned iovcnt;
	} *uap = (struct a *)u.u_arg;
	struct uio auio;
	struct iovec aiov[16];		/* XXX */

	if (uap->iovcnt > sizeof(aiov)/sizeof(aiov[0])) {
		u.u_error = EINVAL;
		return;
	}
	auio.uio_iov = aiov;
	auio.uio_iovcnt = uap->iovcnt;
	auio.uio_rw = UIO_READ;
	u.u_error = copyin ((caddr_t)uap->iovp, (caddr_t)aiov,
		uap->iovcnt * sizeof (struct iovec));
	if (u.u_error)
		return;
	rwuio (&auio);
}

/*
 * Write system call
 */
void
write()
{
	register struct a {
		int	fdes;
		char	*cbuf;
		unsigned count;
	} *uap = (struct a *)u.u_arg;
	struct uio auio;
	struct iovec aiov;

	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_WRITE;
	aiov.iov_base = uap->cbuf;
	aiov.iov_len = uap->count;
	rwuio (&auio);
}

void
writev()
{
	register struct a {
		int	fdes;
		struct	iovec *iovp;
		unsigned iovcnt;
	} *uap = (struct a *)u.u_arg;
	struct uio auio;
	struct iovec aiov[16];		/* XXX */

	if (uap->iovcnt > sizeof(aiov)/sizeof(aiov[0])) {
		u.u_error = EINVAL;
		return;
	}
	auio.uio_iov = aiov;
	auio.uio_iovcnt = uap->iovcnt;
	auio.uio_rw = UIO_WRITE;
	u.u_error = copyin ((caddr_t)uap->iovp, (caddr_t)aiov,
		uap->iovcnt * sizeof (struct iovec));
	if (u.u_error)
		return;
	rwuio (&auio);
}

/*
 * Ioctl system call
 */
void
ioctl()
{
	register struct file *fp;
	register struct a {
		int	fdes;
		long	cmd;
		caddr_t	cmarg;
	} *uap;
	u_int com;

	uap = (struct a *)u.u_arg;
	fp = getf(uap->fdes);
	if (! fp)
		return;
	if (! (fp->f_flag & (FREAD | FWRITE))) {
		u.u_error = EBADF;
		return;
	}
	com = (u_int) uap->cmd;
	if (com & (IOC_IN | IOC_OUT)) {
	        /* Check user address. */
                u_int nbytes = (com & ~(IOC_INOUT | IOC_VOID)) >> 16;
                if (baduaddr (uap->cmarg) ||
                    baduaddr (uap->cmarg + nbytes - 1)) {
                        u.u_error = EFAULT;
                        return;
                }
	}

	switch (com) {
	case FIOCLEX:
		u.u_pofile[uap->fdes] |= UF_EXCLOSE;
		return;
	case FIONCLEX:
		u.u_pofile[uap->fdes] &= ~UF_EXCLOSE;
		return;
	case FIONBIO:
		u.u_error = fset (fp, FNONBLOCK, *(int*) uap->cmarg);
		return;
	case FIOASYNC:
		u.u_error = fset (fp, FASYNC, *(int*) uap->cmarg);
		return;
	case FIOSETOWN:
		u.u_error = fsetown (fp, *(int*) uap->cmarg);
		return;
	case FIOGETOWN:
		u.u_error = fgetown (fp, (int*) uap->cmarg);
		return;
	}
	u.u_error = (*Fops[fp->f_type]->fo_ioctl) (fp, com, uap->cmarg);
}

int	nselcoll;

struct pselect_args {
	int		nd;
	fd_set		*in;
	fd_set		*ou;
	fd_set		*ex;
	struct	timespec *ts;
	sigset_t	*maskp;
};

int
selscan(ibits, obits, nfd, retval)
	fd_set *ibits, *obits;
	int nfd, *retval;
{
	register int i, j, flag;
	fd_mask bits;
	struct file *fp;
	int	which, n = 0;

	for (which = 0; which < 3; which++) {
		switch (which) {
		case 0:
			flag = FREAD; break;
		case 1:
			flag = FWRITE; break;
		case 2:
			flag = 0; break;
		}
		for (i = 0; i < nfd; i += NFDBITS) {
			bits = ibits[which].fds_bits[i/NFDBITS];
			while ((j = ffs(bits)) && i + --j < nfd) {
				bits &= ~(1L << j);
				fp = u.u_ofile[i + j];
				if (fp == NULL)
					return(EBADF);
				if ((*Fops[fp->f_type]->fo_select) (fp, flag)) {
					FD_SET(i + j, &obits[which]);
					n++;
				}
			}
		}
	}
	*retval = n;
	return(0);
}

/*
 * Select helper function common to both select() and pselect()
 */
static int
select1(uap, is_pselect)
	register struct pselect_args *uap;
	int	is_pselect;
{
	fd_set ibits[3], obits[3];
	struct timeval atv;
	sigset_t sigmsk;
	unsigned int timo = 0;
	register int error, ni;
	int ncoll, s;

	bzero((caddr_t)ibits, sizeof(ibits));
	bzero((caddr_t)obits, sizeof(obits));
	if (uap->nd > NOFILE)
		uap->nd = NOFILE;	/* forgiving, if slightly wrong */
	ni = howmany(uap->nd, NFDBITS);

#define	getbits(name, x) \
	if (uap->name) { \
		error = copyin((caddr_t)uap->name, (caddr_t)&ibits[x], \
		    (unsigned)(ni * sizeof(fd_mask))); \
		if (error) \
			goto done; \
	}
	getbits(in, 0);
	getbits(ou, 1);
	getbits(ex, 2);
#undef	getbits

	if (uap->maskp) {
		error = copyin ((caddr_t) uap->maskp, (caddr_t) &sigmsk, sizeof(sigmsk));
		sigmsk &= ~sigcantmask;
		if (error)
			goto done;
	}
	if (uap->ts) {
		error = copyin ((caddr_t) uap->ts, (caddr_t) &atv, sizeof (atv));
		if (error)
			goto done;
		/*
		 * nanoseconds ('struct timespec') on a PDP-11 are stupid since a 50 or 60 hz
		 * clock is all we have.   Keeping the names and logic made porting easier
		 * though.
		 */
		if (is_pselect) {
			struct	timespec *ts = (struct timespec *)&atv;

			if (ts->tv_sec == 0 && ts->tv_nsec < 1000)
				atv.tv_usec = 1;
			else
				atv.tv_usec = ts->tv_nsec / 1000;
		}
		if (itimerfix(&atv)) {
			error = EINVAL;
			goto done;
		}
		s = splhigh();
		time.tv_usec = lbolt * usechz;
		timevaladd(&atv, &time);
		splx(s);
	}
retry:
	ncoll = nselcoll;
	u.u_procp->p_flag |= P_SELECT;
	error = selscan(ibits, obits, uap->nd, &u.u_rval);
	if (error || u.u_rval)
		goto done;
	s = splhigh();
	if (uap->ts) {
		/* this should be timercmp(&time, &atv, >=) */
		if ((time.tv_sec > atv.tv_sec || (time.tv_sec == atv.tv_sec
		    && lbolt * usechz >= atv.tv_usec))) {
			splx(s);
			goto done;
		}
		timo = hzto(&atv);
		if (timo == 0)
			timo = 1;
	}
	if ((u.u_procp->p_flag & P_SELECT) == 0 || nselcoll != ncoll) {
		u.u_procp->p_flag &= ~P_SELECT;
		splx(s);
		goto retry;
	}
	u.u_procp->p_flag &= ~P_SELECT;
	/*
	 * If doing a pselect() need to set a temporary mask while in tsleep.
	 * Returning from pselect after catching a signal the old mask has to be
	 * restored.  Save it here and set the appropriate flag.
	 */
	if (uap->maskp) {
		u.u_oldmask = u.u_procp->p_sigmask;
		u.u_psflags |= SAS_OLDMASK;
		u.u_procp->p_sigmask = sigmsk;
	}
	error = tsleep ((caddr_t) &selwait, PSOCK | PCATCH, timo);
	if (uap->maskp)
		u.u_procp->p_sigmask = u.u_oldmask;
	splx(s);
	if (error == 0)
		goto retry;
done:
	u.u_procp->p_flag &= ~P_SELECT;
	/* select is not restarted after signals... */
	if (error == ERESTART)
		error = EINTR;
	if (error == EWOULDBLOCK)
		error = 0;
#define	putbits(name, x) \
	if (uap->name && \
		(error2 = copyout ((caddr_t) &obits[x], (caddr_t) uap->name, ni*sizeof(fd_mask)))) \
			error = error2;

	if (error == 0) {
		int error2;

		putbits(in, 0);
		putbits(ou, 1);
		putbits(ex, 2);
#undef putbits
	}
	return(error);
}

/*
 * Select system call.
 */
void
select()
{
	struct uap {
		int	nd;
		fd_set	*in, *ou, *ex;
		struct	timeval *tv;
	} *uap = (struct uap *)u.u_arg;
	register struct pselect_args *pselargs = (struct pselect_args *)uap;

	/*
	 * Fake the 6th parameter of pselect.  See the comment below about the
	 * number of parameters!
	*/
	pselargs->maskp = 0;
	u.u_error = select1 (pselargs, 0);
}

/*
 * pselect (posix select)
 *
 * N.B.  There is only room for 6 arguments - see user.h - so pselect() is
 *       at the maximum!  See user.h
 */
void
pselect()
{
	register struct	pselect_args *uap = (struct pselect_args *)u.u_arg;

	u.u_error = select1(uap, 1);
}

/*ARGSUSED*/
int
seltrue(dev, flag)
	dev_t dev;
	int flag;
{
	return (1);
}

void
selwakeup (p, coll)
	register struct proc *p;
	long coll;
{
	if (coll) {
		nselcoll++;
		wakeup ((caddr_t)&selwait);
	}
	if (p) {
		register int s = splhigh();
		if (p->p_wchan == (caddr_t)&selwait) {
			if (p->p_stat == SSLEEP)
				setrun(p);
			else
				unsleep(p);
		} else if (p->p_flag & P_SELECT)
			p->p_flag &= ~P_SELECT;
		splx(s);
	}
}

int
sorw(fp, uio)
	register struct file *fp;
	register struct uio *uio;
{
#ifdef	INET
	if (uio->uio_rw == UIO_READ)
		return(SORECEIVE((struct socket *)fp->f_socket, 0, uio, 0, 0));
	return(SOSEND((struct socket *)fp->f_socket, 0, uio, 0, 0));
#else
	return (EOPNOTSUPP);
#endif
}

int
soctl(fp, com, data)
	struct file *fp;
	u_int	com;
	char	*data;
{
#ifdef	INET
	return (SOO_IOCTL(fp, com, data));
#else
	return (EOPNOTSUPP);
#endif
}

int
sosel(fp, flag)
	struct file *fp;
	int	flag;
{
#ifdef	INET
	return (SOO_SELECT(fp, flag));
#else
	return (EOPNOTSUPP);
#endif
}

int
socls(fp)
	register struct file *fp;
{
	register int error = 0;

#ifdef	INET
	if (fp->f_data)
		error = SOCLOSE((struct socket *)fp->f_data);
	fp->f_data = 0;
#else
	error = EOPNOTSUPP;
#endif
	return(error);
}

/*
 * this is consolidated here rather than being scattered all over the
 * place.  the socketops table has to be in kernel space, but since
 * networking might not be defined an appropriate error has to be set
 */
const struct fileops socketops = {
	sorw, soctl, sosel, socls
};

const struct fileops *const Fops[] = {
	NULL, &inodeops, &socketops, &pipeops
};

/*
 * Routine placed in illegal entries in the bdevsw and cdevsw tables.
 */
void
nostrategy (bp)
	struct buf *bp;
{
	/* Empty. */
}

#ifndef INET
/*
 * socket(2) and socketpair(2) if networking not available.
 */
void
nonet()
{
	u.u_error = EPROTONOSUPPORT;
}
#endif
