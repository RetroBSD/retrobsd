/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "systm.h"
#include "user.h"
#include "proc.h"
#include "inode.h"
#include "file.h"
#include "fs.h"
#include "mount.h"
#include "uio.h"

int
readp (fp, uio, flag)
	register struct file *fp;
	register struct	uio *uio;
	int flag;
{
	register struct inode *ip;
	int error;

	ip = (struct inode *)fp->f_data;
loop:
	/* Very conservative locking. */
	ILOCK(ip);

	/* If nothing in the pipe, wait (unless FNONBLOCK is set). */
	if (ip->i_size == 0) {
		/*
		 * If there are not both reader and writer active,
		 * return without satisfying read.
		 */
		IUNLOCK(ip);
		if (ip->i_count != 2)
			return (0);
		if (fp->f_flag & FNONBLOCK)
			return (EWOULDBLOCK);
		ip->i_mode |= IREAD;
		sleep((caddr_t)ip+4, PPIPE);
		goto loop;
	}

	uio->uio_offset = fp->f_offset;
	error = rwip(ip, uio, flag);
	fp->f_offset = uio->uio_offset;

	/*
	 * If reader has caught up with writer, reset
	 * offset and size to 0.
	 */
	if (fp->f_offset == ip->i_size) {
		fp->f_offset = 0;
		ip->i_size = 0;
		if (ip->i_mode & IWRITE) {
			ip->i_mode &= ~IWRITE;
			wakeup((caddr_t)ip+2);
		}
		if (ip->i_wsel) {
			selwakeup(ip->i_wsel, (long)(ip->i_flag & IWCOLL));
			ip->i_wsel = 0;
			ip->i_flag &= ~IWCOLL;
		}
	}
	IUNLOCK(ip);
	return (error);
}

int
writep (fp, uio, flag)
	struct file *fp;
	register struct	uio *uio;
	int flag;
{
	register struct inode *ip;
	register int c;
	int error = 0;

	ip = (struct inode *)fp->f_data;
	c = uio->uio_resid;
	ILOCK(ip);
	if ((fp->f_flag & FNONBLOCK) && ip->i_size + c >= MAXPIPSIZ) {
		error = EWOULDBLOCK;
		goto done;
	}
loop:
	/* If all done, return. */
	if (c == 0) {
		uio->uio_resid = 0;
		goto done;
	}

	/*
	 * If there are not both read and write sides of the pipe active,
	 * return error and signal too.
	 */
	if (ip->i_count != 2) {
		psignal(u.u_procp, SIGPIPE);
		error = EPIPE;
done:		IUNLOCK(ip);
		return (error);
	}

	/*
	 * If the pipe is full, wait for reads to deplete
	 * and truncate it.
	 */
	if (ip->i_size >= MAXPIPSIZ) {
		ip->i_mode |= IWRITE;
		IUNLOCK(ip);
		sleep((caddr_t)ip+2, PPIPE);
		ILOCK(ip);
		goto loop;
	}

	/*
	 * Write what is possible and loop back.
	 * If writing less than MAXPIPSIZ, it always goes.
	 * One can therefore get a file > MAXPIPSIZ if write
	 * sizes do not divide MAXPIPSIZ.
	 */
	uio->uio_offset = ip->i_size;
	uio->uio_resid = MIN((u_int)c, (u_int)MAXPIPSIZ);
	c -= uio->uio_resid;
	error = rwip(ip, uio, flag);
	if (ip->i_mode&IREAD) {
		ip->i_mode &= ~IREAD;
		wakeup((caddr_t)ip+4);
	}
	if (ip->i_rsel) {
		selwakeup(ip->i_rsel, (long)(ip->i_flag & IRCOLL));
		ip->i_rsel = 0;
		ip->i_flag &= ~IRCOLL;
	}
	goto loop;
}

int
pipe_rw (fp, uio, flag)
	register struct file *fp;
	register struct uio *uio;
	int flag;
{
	if (uio->uio_rw == UIO_READ)
		return (readp(fp, uio, flag));
	return (writep(fp, uio, flag));
}

int
pipe_select (fp, which)
	struct file *fp;
	int which;
{
	register struct inode *ip = (struct inode *)fp->f_data;
	register struct proc *p;
	register int retval = 0;
	extern int selwait;

	ILOCK(ip);
	if (ip->i_count != 2)
		retval = 1;

	else switch (which) {
	case FREAD:
		if (ip->i_size) {
			retval = 1;
			break;
		}
		if ((p = ip->i_rsel) && p->p_wchan == (caddr_t)&selwait)
			ip->i_flag |= IRCOLL;
		else
			ip->i_rsel = u.u_procp;
		break;

	case FWRITE:
		if (ip->i_size < MAXPIPSIZ) {
			retval = 1;
			break;
		}
		if ((p = ip->i_wsel) && p->p_wchan == (caddr_t)&selwait)
			ip->i_flag |= IWCOLL;
		else
			ip->i_wsel = u.u_procp;
		break;
	}
	IUNLOCK(ip);
	return(retval);
}

/*
 * This routine was pulled out of what used to be called 'ino_close'.  Doing
 * so saved a test of the inode belonging to a pipe.   We know this is a pipe
 * because the inode type was DTYPE_PIPE.  The dispatch in closef() can come
 * directly here instead of the general inode close routine.
 *
 * This routine frees the inode by calling 'iput'.  The inode must be
 * unlocked prior to calling this routine because an 'ilock' is done prior
 * to the select wakeup processing.
 */
int
pipe_close(fp)
	struct	file *fp;
{
	register struct inode *ip = (struct inode *)fp->f_data;

	ilock(ip);
#ifdef	DIAGNOSTIC
	if ((ip->i_flag & IPIPE) == 0)
		panic("pipe_close !IPIPE");
#endif
	if (ip->i_rsel) {
		selwakeup(ip->i_rsel, (long)(ip->i_flag & IRCOLL));
		ip->i_rsel = 0;
		ip->i_flag &= ~IRCOLL;
	}
	if (ip->i_wsel) {
		selwakeup(ip->i_wsel, (long)(ip->i_flag & IWCOLL));
		ip->i_wsel = 0;
		ip->i_flag &= ~IWCOLL;
	}
	ip->i_mode &= ~(IREAD|IWRITE);
	wakeup((caddr_t)ip+2);
	wakeup((caddr_t)ip+4);

	/*
	 * And finally decrement the reference count and (likely) release the inode.
	 */
	iput(ip);
	return(0);
}

const struct fileops pipeops = {
	pipe_rw, ino_ioctl, pipe_select, pipe_close
};

/*
 * The sys-pipe entry.
 * Allocate an inode on the root device.  Allocate 2
 * file structures.  Put it all together with flags.
 */
void
pipe()
{
	register struct inode *ip;
	register struct file *rf, *wf;
	static struct mount *mp;
	struct inode itmp;
	int r;

	/*
	 * if pipedev not yet found, or not available, get it; if can't
	 * find it, use rootdev.  It would be cleaner to wander around
	 * and fix it so that this and getfs() only check m_dev OR
	 * m_inodp, but hopefully the mount table isn't scanned enough
	 * to make it a problem.  Besides, 4.3's is just as bad.  Basic
	 * fantasy is that if m_inodp is set, m_dev *will* be okay.
	 */
	if (! mp || ! mp->m_inodp || mp->m_dev != pipedev) {
		for (mp = &mount[0]; ; ++mp) {
			if (mp == &mount[NMOUNT]) {
				mp = &mount[0];		/* use root */
				break;
			}
			if (mp->m_inodp == NULL || mp->m_dev != pipedev)
				continue;
			break;
		}
		if (mp->m_filsys.fs_ronly) {
			u.u_error = EROFS;
			return;
		}
	}
	itmp.i_fs = &mp->m_filsys;
	itmp.i_dev = mp->m_dev;
	ip = ialloc (&itmp);
	if (ip == NULL)
		return;
	rf = falloc();
	if (rf == NULL) {
		iput (ip);
		return;
	}
	r = u.u_rval;
	wf = falloc();
	if (wf == NULL) {
		rf->f_count = 0;
		u.u_ofile[r] = NULL;
		iput (ip);
		return;
	}
#ifdef __mips__
	/* Move a secondary return value to register $v1. */
	u.u_frame [FRAME_R3] = u.u_rval;
#else
#error "pipe return value for unknown architecture"
#endif
	u.u_rval = r;
	wf->f_flag = FWRITE;
	rf->f_flag = FREAD;
	rf->f_type = wf->f_type = DTYPE_PIPE;
	rf->f_data = wf->f_data = (caddr_t) ip;
	ip->i_count = 2;
	ip->i_mode = IFREG;
	ip->i_flag = IACC | IUPD | ICHG | IPIPE;
}
