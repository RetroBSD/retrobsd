/*
 * sysctl system call.
 *
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Karels at Berkeley Software Design, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/param.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/inode.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/vm.h>
#include <sys/map.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>
#include <conf.h>

sysctlfn kern_sysctl;
sysctlfn hw_sysctl;
#ifdef DEBUG
sysctlfn debug_sysctl;
#endif
sysctlfn vm_sysctl;
sysctlfn fs_sysctl;
#ifdef	INET
sysctlfn net_sysctl;
#endif
sysctlfn cpu_sysctl;

struct sysctl_args {
	int	*name;
	u_int	namelen;
	void	*old;
	size_t	*oldlenp;
	void	*new;
	size_t	newlen;
};

static int sysctl_clockrate (char *where, size_t *sizep);
static int sysctl_inode (char *where, size_t *sizep);
static int sysctl_file (char *where, size_t *sizep);
static int sysctl_doproc (int *name, u_int namelen, char *where, size_t *sizep);

void
__sysctl()
{
	register struct sysctl_args *uap = (struct sysctl_args*) u.u_arg;
	int error;
	u_int oldlen = 0;
	sysctlfn *fn;
	int name [CTL_MAXNAME];

	if (uap->new != NULL && ! suser())
		return;
	/*
	 * all top-level sysctl names are non-terminal
	 */
	if (uap->namelen > CTL_MAXNAME || uap->namelen < 2) {
		u.u_error = EINVAL;
		return;
	}
	error = copyin ((caddr_t) uap->name, (caddr_t) &name, uap->namelen * sizeof(int));
	if (error) {
		u.u_error = error;
		return;
	}

	switch (name[0]) {
	case CTL_KERN:
		fn = kern_sysctl;
		break;
	case CTL_HW:
		fn = hw_sysctl;
		break;
	case CTL_VM:
		fn = vm_sysctl;
		break;
#ifdef	INET
	case CTL_NET:
		fn = net_sysctl;
		break;
#endif
#ifdef notyet
	case CTL_FS:
		fn = fs_sysctl;
		break;
#endif
	case CTL_MACHDEP:
		fn = cpu_sysctl;
		break;
#ifdef DEBUG
	case CTL_DEBUG:
		fn = debug_sysctl;
		break;
#endif
	default:
		u.u_error = EOPNOTSUPP;
		return;
	}

	if (uap->oldlenp && (error = copyin ((caddr_t) uap->oldlenp,
	    (caddr_t) &oldlen, sizeof(oldlen)))) {
		u.u_error = error;
		return;
	}
	if (uap->old != NULL) {
		while (memlock.sl_lock) {
			memlock.sl_want = 1;
			sleep((caddr_t)&memlock, PRIBIO+1);
			memlock.sl_locked++;
		}
		memlock.sl_lock = 1;
	}
	error = (*fn) (name + 1, uap->namelen - 1, uap->old, &oldlen,
		uap->new, uap->newlen);
	if (uap->old != NULL) {
		memlock.sl_lock = 0;
		if (memlock.sl_want) {
			memlock.sl_want = 0;
			wakeup((caddr_t)&memlock);
		}
	}
	if (error) {
		u.u_error = error;
		return;
	}
	if (uap->oldlenp) {
		error = copyout ((caddr_t) &oldlen, (caddr_t) uap->oldlenp, sizeof(oldlen));
		if (error) {
			u.u_error = error;
			return;
		}
	}
	u.u_rval = oldlen;
}

/*
 * kernel related system variables.
 */
int
kern_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	int error, level;
	u_long longhostid;
	char bsd[10];

	/* all sysctl names at this level are terminal */
	if (namelen != 1 && !(name[0] == KERN_PROC || name[0] == KERN_PROF))
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case KERN_OSTYPE:
	case KERN_OSRELEASE:
		/* code is cheaper than D space */
		bsd[0]='2';bsd[1]='.';bsd[2]='1';bsd[3]='1';bsd[4]='B';
		bsd[5]='S';bsd[6]='D';bsd[7]='\0';
		return (sysctl_rdstring(oldp, oldlenp, newp, bsd));
	case KERN_OSREV:
		return (sysctl_rdlong(oldp, oldlenp, newp, (long)BSD));
	case KERN_VERSION:
		return (sysctl_rdstring(oldp, oldlenp, newp, version));
	case KERN_MAXINODES:
		return(sysctl_rdint(oldp, oldlenp, newp, NINODE));
	case KERN_MAXPROC:
		return (sysctl_rdint(oldp, oldlenp, newp, NPROC));
	case KERN_MAXFILES:
		return (sysctl_rdint(oldp, oldlenp, newp, NFILE));
	case KERN_ARGMAX:
		return (sysctl_rdint(oldp, oldlenp, newp, NCARGS));
	case KERN_SECURELVL:
		level = securelevel;
		if ((error = sysctl_int(oldp, oldlenp, newp, newlen, &level)) ||
		    newp == NULL)
			return (error);
		if (level < securelevel && u.u_procp->p_pid != 1)
			return (EPERM);
		securelevel = level;
		return (0);
	case KERN_HOSTNAME:
		error = sysctl_string(oldp, oldlenp, newp, newlen,
		    hostname, sizeof(hostname));
		if (newp && !error)
			hostnamelen = newlen;
		return (error);
	case KERN_HOSTID:
		longhostid = hostid;
		error =  sysctl_long(oldp, oldlenp, newp, newlen, (long*) &longhostid);
		hostid = longhostid;
		return (error);
	case KERN_CLOCKRATE:
		return (sysctl_clockrate(oldp, oldlenp));
	case KERN_BOOTTIME:
		return (sysctl_rdstruct(oldp, oldlenp, newp, &boottime,
		    sizeof(struct timeval)));
	case KERN_INODE:
		return (sysctl_inode(oldp, oldlenp));
	case KERN_PROC:
		return (sysctl_doproc(name + 1, namelen - 1, oldp, oldlenp));
	case KERN_FILE:
		return (sysctl_file(oldp, oldlenp));
#ifdef GPROF
	case KERN_PROF:
		return (sysctl_doprof(name + 1, namelen - 1, oldp, oldlenp,
		    newp, newlen));
#endif
	case KERN_NGROUPS:
		return (sysctl_rdint(oldp, oldlenp, newp, NGROUPS));
	case KERN_JOB_CONTROL:
		return (sysctl_rdint(oldp, oldlenp, newp, 1));
	case KERN_POSIX1:
	case KERN_SAVED_IDS:
		return (sysctl_rdint(oldp, oldlenp, newp, 0));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * hardware related system variables.
 */
int
hw_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case HW_MACHINE:
		return (sysctl_rdstring(oldp, oldlenp, newp, "pic32"));
	case HW_MODEL:
		return (sysctl_rdstring(oldp, oldlenp, newp, "mips"));
	case HW_NCPU:
		return (sysctl_rdint(oldp, oldlenp, newp, 1));	/* XXX */
	case HW_BYTEORDER:
		return (sysctl_rdint(oldp, oldlenp, newp, ENDIAN));
	case HW_PHYSMEM:
		return (sysctl_rdlong(oldp, oldlenp, newp, physmem));
#ifdef UCB_METER
	case HW_USERMEM:
		return (sysctl_rdlong(oldp, oldlenp, newp, freemem));
#endif
	case HW_PAGESIZE:
		return (sysctl_rdint(oldp, oldlenp, newp, DEV_BSIZE));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

#ifdef DEBUG
/*
 * Debugging related system variables.
 */
struct ctldebug debug0, debug1, debug2, debug3, debug4;
struct ctldebug debug5, debug6, debug7, debug8, debug9;
struct ctldebug debug10, debug11, debug12, debug13, debug14;
struct ctldebug debug15, debug16, debug17, debug18, debug19;
static struct ctldebug *debugvars[CTL_DEBUG_MAXID] = {
	&debug0, &debug1, &debug2, &debug3, &debug4,
	&debug5, &debug6, &debug7, &debug8, &debug9,
	&debug10, &debug11, &debug12, &debug13, &debug14,
	&debug15, &debug16, &debug17, &debug18, &debug19,
};

int
debug_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	struct ctldebug *cdp;

	/* all sysctl names at this level are name and field */
	if (namelen != 2)
		return (ENOTDIR);		/* overloaded */
	cdp = debugvars[name[0]];
	if (cdp->debugname == 0)
		return (EOPNOTSUPP);
	switch (name[1]) {
	case CTL_DEBUG_NAME:
		return (sysctl_rdstring(oldp, oldlenp, newp, cdp->debugname));
	case CTL_DEBUG_VALUE:
		return (sysctl_int(oldp, oldlenp, newp, newlen, cdp->debugvar));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}
#endif /* DEBUG */

/*
 * Bit of a hack.  2.11 currently uses 'short avenrun[3]' and a fixed scale
 * of 256.  In order not to break all the applications which nlist() for
 * 'avenrun' we build a local 'averunnable' structure here to return to the
 * user.  Eventually (after all applications which look up the load average
 * the old way) have been converted we can change things.
 *
 * We do not call vmtotal(), that could get rather expensive, rather we rely
 * on the 5 second update.
 *
 * The swapmap case is 2.11BSD extension.
 */
int
vm_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	struct	loadavg averunnable;	/* loadavg in resource.h */

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case VM_LOADAVG:
		averunnable.fscale = 256;
		averunnable.ldavg[0] = avenrun[0];
		averunnable.ldavg[1] = avenrun[1];
		averunnable.ldavg[2] = avenrun[2];
		return (sysctl_rdstruct(oldp, oldlenp, newp, &averunnable,
		    sizeof(averunnable)));
	case VM_METER:
#ifdef	notsure
		vmtotal();	/* could be expensive to do this every time */
#endif
		return (sysctl_rdstruct(oldp, oldlenp, newp, &total,
		    sizeof(total)));
	case VM_SWAPMAP:
		if (oldp == NULL) {
			*oldlenp = (char *)swapmap[0].m_limit -
					(char *)swapmap[0].m_map;
			return(0);
		}
		return (sysctl_rdstruct(oldp, oldlenp, newp, swapmap,
			(int)swapmap[0].m_limit - (int)swapmap[0].m_map));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * Validate parameters and get old / set new parameters
 * for an integer-valued sysctl function.
 */
int
sysctl_int(oldp, oldlenp, newp, newlen, valp)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	int *valp;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(int))
		return (ENOMEM);
	if (newp && newlen != sizeof(int))
		return (EINVAL);
	*oldlenp = sizeof(int);
	if (oldp)
		error = copyout ((caddr_t) valp, (caddr_t) oldp, sizeof(int));
	if (error == 0 && newp)
		error = copyin ((caddr_t) newp, (caddr_t) valp, sizeof(int));
	return (error);
}

/*
 * As above, but read-only.
 */
int
sysctl_rdint(oldp, oldlenp, newp, val)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	int val;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(int))
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = sizeof(int);
	if (oldp)
		error = copyout((caddr_t)&val, oldp, sizeof(int));
	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for an long-valued sysctl function.
 */
int
sysctl_long(oldp, oldlenp, newp, newlen, valp)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	long *valp;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(long))
		return (ENOMEM);
	if (newp && newlen != sizeof(long))
		return (EINVAL);
	*oldlenp = sizeof(long);
	if (oldp)
		error = copyout ((caddr_t) valp, (caddr_t) oldp, sizeof(long));
	if (error == 0 && newp)
		error = copyin ((caddr_t) newp, (caddr_t) valp, sizeof(long));
	return (error);
}

/*
 * As above, but read-only.
 */
int
sysctl_rdlong(oldp, oldlenp, newp, val)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	long val;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(long))
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = sizeof(long);
	if (oldp)
		error = copyout((caddr_t)&val, oldp, sizeof(long));
	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for a string-valued sysctl function.
 */
int
sysctl_string(oldp, oldlenp, newp, newlen, str, maxlen)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	char *str;
	int maxlen;
{
	int len, error = 0;

	len = strlen(str) + 1;
	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp && newlen >= maxlen)
		return (EINVAL);
	if (oldp) {
		*oldlenp = len;
		error = copyout (str, oldp, len);
	}
	if (error == 0 && newp) {
		error = copyin (newp, str, newlen);
		str[newlen] = 0;
	}
	return (error);
}

/*
 * As above, but read-only.
 */
int
sysctl_rdstring(oldp, oldlenp, newp, str)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	const char *str;
{
	int len, error = 0;

	len = strlen(str) + 1;
	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = len;
	if (oldp)
		error = copyout ((caddr_t) str, oldp, len);
	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for a structure oriented sysctl function.
 */
int
sysctl_struct(oldp, oldlenp, newp, newlen, sp, len)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	void *sp;
	int len;
{
	int error = 0;

	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp && newlen > len)
		return (EINVAL);
	if (oldp) {
		*oldlenp = len;
		error = copyout(sp, oldp, len);
	}
	if (error == 0 && newp)
		error = copyin(newp, sp, len);
	return (error);
}

/*
 * Validate parameters and get old parameters
 * for a structure oriented sysctl function.
 */
int
sysctl_rdstruct(oldp, oldlenp, newp, sp, len)
	void *oldp;
	size_t *oldlenp;
	void *newp, *sp;
	int len;
{
	int error = 0;

	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = len;
	if (oldp)
		error = copyout(sp, oldp, len);
	return (error);
}

/*
 * Get file structures.
 */
int
sysctl_file(where, sizep)
	char *where;
	size_t *sizep;
{
	int buflen, error;
	register struct file *fp;
	struct	file *fpp;
	char *start = where;
	register int i;

	buflen = *sizep;
	if (where == NULL) {
		for (i = 0, fp = file; fp < file+NFILE; fp++)
			if (fp->f_count) i++;

#define	FPTRSZ	sizeof (struct file *)
#define	FILESZ	sizeof (struct file)
		/*
		 * overestimate by 5 files
		 */
		*sizep = (i + 5) * (FILESZ + FPTRSZ);
		return (0);
	}

	/*
	 * array of extended file structures: first the address then the
	 * file structure.
	 */
	for (fp = file; fp < file+NFILE; fp++) {
		if (fp->f_count == 0)
			continue;
		if (buflen < (FPTRSZ + FILESZ)) {
			*sizep = where - start;
			return (ENOMEM);
		}
		fpp = fp;
		if ((error = copyout ((caddr_t) &fpp, (caddr_t) where, FPTRSZ)) ||
		    (error = copyout ((caddr_t) fp, (caddr_t) (where + FPTRSZ), FILESZ)))
			return (error);
		buflen -= (FPTRSZ + FILESZ);
		where += (FPTRSZ + FILESZ);
	}
	*sizep = where - start;
	return (0);
}

/*
 * This one is in kern_clock.c in 4.4 but placed here for the reasons
 * given earlier (back around line 367).
 */
int
sysctl_clockrate (where, sizep)
	char *where;
	size_t *sizep;
{
	struct	clockinfo clkinfo;

	/*
	 * Construct clockinfo structure.
	*/
	clkinfo.hz = hz;
	clkinfo.tick = usechz;
	clkinfo.profhz = 0;
	clkinfo.stathz = hz;
	return(sysctl_rdstruct(where, sizep, NULL, &clkinfo, sizeof (clkinfo)));
}

/*
 * Dump inode list (via sysctl).
 * Copyout address of inode followed by inode.
 */
/* ARGSUSED */
int
sysctl_inode (where, sizep)
	char *where;
	size_t *sizep;
{
	register struct inode *ip;
	register char *bp = where;
	struct inode *ipp;
	char *ewhere;
	int error, numi;

	for (numi = 0, ip = inode; ip < inode+NINODE; ip++)
		if (ip->i_count) numi++;

#define IPTRSZ	sizeof (struct inode *)
#define INODESZ	sizeof (struct inode)
	if (where == NULL) {
		*sizep = (numi + 5) * (IPTRSZ + INODESZ);
		return (0);
	}
	ewhere = where + *sizep;

	for (ip = inode; ip < inode+NINODE; ip++) {
		if (ip->i_count == 0)
			continue;
		if (bp + IPTRSZ + INODESZ > ewhere) {
			*sizep = bp - where;
			return (ENOMEM);
		}
		ipp = ip;
		if ((error = copyout ((caddr_t)&ipp, bp, IPTRSZ)) ||
		    (error = copyout ((caddr_t)ip, bp + IPTRSZ, INODESZ)))
			return (error);
		bp += IPTRSZ + INODESZ;
	}

	*sizep = bp - where;
	return (0);
}

/*
 * Three pieces of information we need about a process are not kept in
 * the proc table: real uid, controlling terminal device, and controlling
 * terminal tty struct pointer.  For these we must look in either the u
 * area or the swap area.  If the process is still in memory this is
 * easy but if the process has been swapped out we have to read in the
 * u area.
 *
 * XXX - We rely on the fact that u_ttyp, u_ttyd, and u_ruid are all within
 * XXX - the first 1kb of the u area.  If this ever changes the logic below
 * XXX - will break (and badly).  At the present time (97/9/2) the u area
 * XXX - is 856 bytes long.
 */
void
fill_from_u (p, rup, ttp, tdp)
	struct	proc	*p;
	uid_t	*rup;
	struct	tty	**ttp;
	dev_t	*tdp;
{
	register struct	buf	*bp;
	dev_t	ttyd;
	uid_t	ruid;
	struct	tty	*ttyp;
	struct	user	*up;

	if (p->p_stat == SZOMB) {
		ruid = (uid_t)-2;
		ttyp = NULL;
		ttyd = NODEV;
		goto out;
	}
	if (p->p_flag & SLOAD) {
		ttyd = ((struct user *)p->p_addr)->u_ttyd;
		ttyp = ((struct user *)p->p_addr)->u_ttyp;
		ruid = ((struct user *)p->p_addr)->u_ruid;
	} else {
		bp = geteblk();
		bp->b_dev = swapdev;
		bp->b_blkno = (daddr_t)p->p_addr;
		bp->b_bcount = DEV_BSIZE;	/* XXX */
		bp->b_flags = B_READ;

		(*bdevsw[major(swapdev)].d_strategy)(bp);
		biowait(bp);

		if (u.u_error) {
			ttyd = NODEV;
			ttyp = NULL;
			ruid = (uid_t)-2;
		} else {
			up = (struct user*) bp->b_addr;
			ruid = up->u_ruid;	/* u_ruid = offset 164 */
			ttyd = up->u_ttyd;	/* u_ttyd = offset 654 */
			ttyp = up->u_ttyp;	/* u_ttyp = offset 652 */
		}
		bp->b_flags |= B_AGE;
		brelse(bp);
		u.u_error = 0;		/* XXX */
	}
out:
	if (rup)
		*rup = ruid;
	if (ttp)
		*ttp = ttyp;
	if (tdp)
		*tdp = ttyd;
}

/*
 * Fill in an eproc structure for the specified process.  Slightly
 * inefficient because we have to access the u area again for the
 * information not kept in the proc structure itself.  Can't afford
 * to expand the proc struct so we take a slight speed hit here.
 */
static void
fill_eproc(p, ep)
	register struct proc *p;
	register struct eproc *ep;
{
	struct	tty	*ttyp;

	ep->e_paddr = p;
	fill_from_u(p, &ep->e_ruid, &ttyp, &ep->e_tdev);
	if	(ttyp)
		ep->e_tpgid = ttyp->t_pgrp;
	else
		ep->e_tpgid = 0;
}

/*
 * try over estimating by 5 procs
 */
#define KERN_PROCSLOP	(5 * sizeof (struct kinfo_proc))

int
sysctl_doproc(name, namelen, where, sizep)
	int *name;
	u_int namelen;
	char *where;
	size_t *sizep;
{
	register struct proc *p;
	register struct kinfo_proc *dp = (struct kinfo_proc *)where;
	int needed = 0;
	int buflen = where != NULL ? *sizep : 0;
	int doingzomb;
	struct eproc eproc;
	int error = 0;
	dev_t ttyd;
	uid_t ruid;
	struct tty *ttyp;

	if (namelen != 2 && !(namelen == 1 && name[0] == KERN_PROC_ALL))
		return (EINVAL);
	p = (struct proc *)allproc;
	doingzomb = 0;
again:
	for (; p != NULL; p = p->p_nxt) {
		/*
		 * Skip embryonic processes.
		 */
		if (p->p_stat == SIDL)
			continue;
		/*
		 * TODO: sysctl_oproc - make more efficient (see notes below).
		 * do by session.
		 */
		switch (name[0]) {

		case KERN_PROC_PID:
			/* could do this with just a lookup */
			if (p->p_pid != (pid_t)name[1])
				continue;
			break;

		case KERN_PROC_PGRP:
			/* could do this by traversing pgrp */
			if (p->p_pgrp != (pid_t)name[1])
				continue;
			break;

		case KERN_PROC_TTY:
			fill_from_u(p, &ruid, &ttyp, &ttyd);
			if (!ttyp || ttyd != (dev_t)name[1])
				continue;
			break;

		case KERN_PROC_UID:
			if (p->p_uid != (uid_t)name[1])
				continue;
			break;

		case KERN_PROC_RUID:
			fill_from_u(p, &ruid, &ttyp, &ttyd);
			if (ruid != (uid_t)name[1])
				continue;
			break;

		case KERN_PROC_ALL:
			break;
		default:
			return(EINVAL);
		}
		if (buflen >= sizeof(struct kinfo_proc)) {
			fill_eproc(p, &eproc);
			error = copyout ((caddr_t) p, (caddr_t) &dp->kp_proc,
				sizeof(struct proc));
			if (error)
				return (error);
			error = copyout ((caddr_t)&eproc, (caddr_t) &dp->kp_eproc,
				sizeof(eproc));
			if (error)
				return (error);
			dp++;
			buflen -= sizeof(struct kinfo_proc);
		}
		needed += sizeof(struct kinfo_proc);
	}
	if (doingzomb == 0) {
		p = zombproc;
		doingzomb++;
		goto again;
	}
	if (where != NULL) {
		*sizep = (caddr_t)dp - where;
		if (needed > *sizep)
			return (ENOMEM);
	} else {
		needed += KERN_PROCSLOP;
		*sizep = needed;
	}
	return (0);
}
