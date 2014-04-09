/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "systm.h"
#include "map.h"
#include "user.h"
#include "proc.h"
#include "buf.h"
#include "inode.h"
#include "namei.h"
#include "fs.h"
#include "mount.h"
#include "file.h"
#include "signalvar.h"

/*
 * exec system call, with and without environments.
 */
struct execa {
	char	*fname;
	char	**argp;
	char	**envp;
};

/*
 * Reset signals for an exec of the specified process.  In 4.4 this function
 * was in kern_sig.c but since in 2.11 kern_sig and kern_exec will likely be
 * in different overlays placing this here potentially saves a kernel overlay
 * switch.
 */
static void
execsigs(p)
	register struct proc *p;
{
	register int nc;
	unsigned long mask;

	/*
	 * Reset caught signals.  Held signals remain held
	 * through p_sigmask (unless they were caught,
	 * and are now ignored by default).
	 */
	while (p->p_sigcatch) {
		nc = ffs(p->p_sigcatch);
		mask = sigmask(nc);
		p->p_sigcatch &= ~mask;
		if (sigprop[nc] & SA_IGNORE) {
			if (nc != SIGCONT)
				p->p_sigignore |= mask;
			p->p_sig &= ~mask;
		}
		u.u_signal[nc] = SIG_DFL;
	}
	/*
	 * Reset stack state to the user stack (disable the alternate stack).
	 */
	u.u_sigstk.ss_flags = SA_DISABLE;
	u.u_sigstk.ss_size = 0;
	u.u_sigstk.ss_base = 0;
	u.u_psflags = 0;
}

/*
 * Read in and set up memory for executed file.
 * u.u_error set on error
 */
static void
getxfile (ip, ep, nargc, uid, gid)
	struct inode *ip;
	register struct exec *ep;
	int nargc, uid, gid;
{
	u_int ds, ts, ss;

	if (ep->a_magic == OMAGIC) {
		ep->a_data += ep->a_text;
		ep->a_text = 0;
	}

	if (ep->a_text != 0 && (ip->i_flag & ITEXT) == 0 &&
	    ip->i_count != 1) {
		register struct file *fp;

		for (fp = file; fp < file+NFILE; fp++) {
			if (fp->f_type == DTYPE_INODE &&
			    fp->f_count > 0 &&
			    (struct inode*)fp->f_data == ip &&
			    (fp->f_flag & FWRITE)) {
				u.u_error = ETXTBSY;
				return;
			}
		}
	}

	/*
	 * find text and data sizes try; them out for possible
	 * overflow of max sizes
	 */
	ts = ep->a_text;
	ds = ep->a_data + ep->a_bss;
	ss = SSIZE + nargc;

//printf ("getxfile: size t/d/s = %u/%u/%u\n", ts, ds, ss);
	if (ts + ds + ss > MAXMEM) {
		u.u_error = ENOMEM;
		return;
	}

	/*
	 * Allocate core at this point, committed to the new image.
	 */
	u.u_prof.pr_scale = 0;
	if (u.u_procp->p_flag & SVFORK)
		endvfork();
	u.u_procp->p_dsize = ds;
	u.u_procp->p_daddr = USER_DATA_START;
	u.u_procp->p_ssize = ss;
	u.u_procp->p_saddr = USER_DATA_END - ss;

	/* read in text and data */
//printf ("getxfile: read %u bytes at %08x\n", ep->a_data, USER_DATA_START);
	rdwri (UIO_READ, ip, (caddr_t) USER_DATA_START, ep->a_data,
                sizeof(struct exec) + ep->a_text, IO_UNIT, (int*) 0);

	/* clear BSS and stack */
//printf ("getxfile: clear %u bytes at %08x\n", USER_DATA_END - USER_DATA_START - ep->a_data, USER_DATA_START + ep->a_data);
	bzero ((void*) (USER_DATA_START + ep->a_data),
            USER_DATA_END - USER_DATA_START - ep->a_data);

	/*
	 * set SUID/SGID protections, if no tracing
	 */
	if ((u.u_procp->p_flag & P_TRACED) == 0) {
		u.u_uid = uid;
		u.u_procp->p_uid = uid;
		u.u_groups[0] = gid;
	} else
		psignal (u.u_procp, SIGTRAP);
	u.u_svuid = u.u_uid;
	u.u_svgid = u.u_groups[0];

	u.u_tsize = ts;
	u.u_dsize = ds;
	u.u_ssize = ss;
}

void printmem (unsigned addr, int nbytes)
{
        for (; nbytes>0; addr+=16, nbytes-=16) {
                unsigned char *p = (unsigned char*) addr;
                printf ("%08x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    addr, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
                    p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
        }
}

void
execv()
{
	struct execa *arg = (struct execa *)u.u_arg;

	arg->envp = NULL;
	execve();
}

void
execve()
{
	register char *cp;
	register struct buf *bp;
	struct execa *uap = (struct execa *)u.u_arg;
	int nc, na, ne, ucp, ap, indir, uid, gid, resid, error;
	register int cc;
	unsigned len;
	char *sharg;
	struct inode *ip;
	size_t bno;
	char cfname [MAXCOMLEN + 1];
#define	SHSIZE	32
	char cfarg [SHSIZE];
	union {
		char	ex_shell [SHSIZE];	/* #! and name of interpreter */
		struct	exec ex_exec;
	} exdata;
	struct nameidata nd;
	register struct	nameidata *ndp = &nd;

//printf ("execve ('%s', ['%s', '%s', ...])\n", uap->fname, uap->argp[0], uap->argp[1]);
	NDINIT (ndp, LOOKUP, FOLLOW, uap->fname);
	ip = namei (ndp);
	if (ip == NULL) {
//printf ("execve: file not found\n");
		return;
        }
	bno = 0;
	bp = 0;
	indir = 0;
	uid = u.u_uid;
	gid = u.u_groups[0];
	if (ip->i_fs->fs_flags & MNT_NOEXEC) {
//printf ("execve: NOEXEC, flags=%o\n", ip->i_fs->fs_flags);
		u.u_error = EACCES;
		goto done;
	}
	if ((ip->i_fs->fs_flags & MNT_NOSUID) == 0) {
		if (ip->i_mode & ISUID)
			uid = ip->i_uid;
		if (ip->i_mode & ISGID)
			gid = ip->i_gid;
	}
again:
	if (access (ip, IEXEC)) {
//printf ("execve: no IEXEC\n");
		goto done;
        }
	if ((u.u_procp->p_flag & P_TRACED) && access (ip, IREAD)) {
//printf ("execve: traced, but no IREAD\n");
		goto done;
        }
	if ((ip->i_mode & IFMT) != IFREG ||
	    (ip->i_mode & (IEXEC | (IEXEC>>3) | (IEXEC>>6))) == 0) {
//printf ("execve: no IEXEC, mode=%o\n", ip->i_mode);
		u.u_error = EACCES;
		goto done;
	}

	/*
	 * Read in first few bytes of file for segment sizes, magic number:
	 *	407 = plain executable
	 * Also an ASCII line beginning with #! is
	 * the file name of a ``shell'' and arguments may be prepended
	 * to the argument list if given here.
	 *
	 * SHELL NAMES ARE LIMITED IN LENGTH.
	 *
	 * ONLY ONE ARGUMENT MAY BE PASSED TO THE SHELL FROM
	 * THE ASCII LINE.
	 */
	exdata.ex_shell[0] = '\0';	/* for zero length files */
	u.u_error = rdwri (UIO_READ, ip, (caddr_t) &exdata, sizeof(exdata),
				(off_t) 0, IO_UNIT, &resid);
	if (u.u_error) {
//printf ("execve: rdwri error %d\n", u.u_error);
		goto done;
        }
	if (resid > sizeof(exdata) - sizeof(exdata.ex_exec) &&
	    exdata.ex_shell[0] != '#') {
//printf ("execve: short read, resid = %d, shell=%.32s\n", resid, exdata.ex_shell);
		u.u_error = ENOEXEC;
		goto done;
	}
//printf ("execve: text=%u, data=%u, bss=%u\n", exdata.ex_exec.a_text, exdata.ex_exec.a_data, exdata.ex_exec.a_bss);

	switch ((int) exdata.ex_exec.a_magic) {
	case OMAGIC:
	case NMAGIC:
		break;
	default:
		if (exdata.ex_shell[0] != '#' ||
		    exdata.ex_shell[1] != '!' ||
		    indir) {
//printf ("execve: bad shell=%.32s\n", exdata.ex_shell);
			u.u_error = ENOEXEC;
			goto done;
		}
		/*
		 * If setuid/gid scripts were to be disallowed this is where it would
		 * have to be done.
		 * u.u_uid = uid;
		 * u.u_gid = u_groups[0];
		 */
		cp = &exdata.ex_shell[2];		/* skip "#!" */
		while (cp < &exdata.ex_shell[SHSIZE]) {
			if (*cp == '\t')
				*cp = ' ';
			else if (*cp == '\n') {
				*cp = '\0';
				break;
			}
			cp++;
		}
		if (*cp != '\0') {
			u.u_error = ENOEXEC;
			goto done;
		}
		cp = &exdata.ex_shell[2];
		while (*cp == ' ')
			cp++;
		ndp->ni_dirp = cp;
		while (*cp && *cp != ' ')
			cp++;
		cfarg[0] = '\0';
		if (*cp) {
			*cp++ = '\0';
			while (*cp == ' ')
				cp++;
			if (*cp)
				bcopy ((caddr_t) cp, (caddr_t) cfarg, SHSIZE);
		}
		indir = 1;
		iput (ip);
		ndp->ni_nameiop = LOOKUP | FOLLOW;
		ip = namei (ndp);
		if (ip == NULL)
			return;
		bcopy ((caddr_t) ndp->ni_dent.d_name, (caddr_t) cfname, MAXCOMLEN);
		cfname [MAXCOMLEN] = '\0';
		goto again;
	}

	/*
	 * Collect arguments on "file" in swap space.
	 */
	na = 0;
	ne = 0;
	nc = 0;
	cc = 0;
	cp = 0;
	bno = malloc (swapmap, btod (NCARGS + MAXBSIZE));
	if (bno == 0) {
		u.u_error = ENOMEM;
		goto done;
	}
	/*
	 * Copy arguments into file in argdev area.
	 */
	if (uap->argp) for (;;) {
		ap = NULL;
		sharg = NULL;
		if (indir && na == 0) {
			sharg = cfname;
			ap = (int) sharg;
			uap->argp++;		/* ignore argv[0] */
		} else if (indir && (na == 1 && cfarg[0])) {
			sharg = cfarg;
			ap = (int) sharg;
		} else if (indir && (na == 1 || (na == 2 && cfarg[0])))
			ap = (int) uap->fname;
		else if (uap->argp) {
			ap = *(int*) uap->argp;
			uap->argp++;
		}
		if (ap == NULL && uap->envp) {
			uap->argp = NULL;
			ap = *(int*) uap->envp;
			if (ap != NULL)
				uap->envp++, ne++;
		}
		if (ap == NULL)
			break;
		na++;
		if (ap == -1) {
			u.u_error = EFAULT;
			break;
		}
		do {
			if (cc <= 0) {
				/*
				 * We depend on NCARGS being a multiple of
				 * DEV_BSIZE.  This way we need only check
				 * overflow before each buffer allocation.
				 */
				if (nc >= NCARGS-1) {
//printf ("execve: too many args = %d\n", nc);
					error = E2BIG;
					break;
				}
				if (bp) {
					bdwrite(bp);
				}
				cc = DEV_BSIZE;
				bp = getblk (swapdev, dbtofsb(bno) + lblkno(nc));
				cp = bp->b_addr;
			}
			if (sharg) {
				error = copystr (sharg, cp, (unsigned) cc, &len);
//printf ("execve arg%d=%s: %u bytes from %08x to %08x\n", na-1, sharg, len, sharg, cp);
				sharg += len;
			} else {
				error = copystr ((caddr_t) ap, cp, (unsigned) cc, &len);
//printf ("execve arg%d=%s: %u bytes from %08x to %08x\n", na-1, ap, len, ap, cp);
				ap += len;
			}
			cp += len;
			nc += len;
			cc -= len;
		} while (error == ENOENT);
		if (error) {
//printf ("execve: copy arg error = %d\n", error);
			u.u_error = error;
			if (bp) {
				bp->b_flags |= B_AGE;
				bp->b_flags &= ~B_DELWRI;
				brelse(bp);
			}
			bp = 0;
			goto badarg;
		}
	}
//printf ("execve: argc=%d, envc=%d, total %d bytes\n", na, ne, nc);
	if (bp) {
		bdwrite (bp);
	}
	bp = 0;
	nc = (nc + NBPW-1) & ~(NBPW-1);
	getxfile (ip, &exdata.ex_exec, nc + (na+4)*NBPW, uid, gid);
	if (u.u_error) {
//printf ("execve: getxfile error = %d\n", u.u_error);
badarg:
		for (cc = 0; cc < nc; cc += DEV_BSIZE) {
			daddr_t blkno;

			blkno = dbtofsb(bno) + lblkno(cc);
			if (incore (swapdev, blkno)) {
				bp = bread (swapdev, blkno);
				bp->b_flags |= B_AGE;		/* throw away */
				bp->b_flags &= ~B_DELWRI;	/* cancel io */
				brelse(bp);
				bp = 0;
			}
		}
		goto done;
	}
	iput(ip);
	ip = NULL;

	/*
	 * Copy back arglist.
	 */
	ucp = USER_DATA_END - nc - NBPW;
	ap = ucp - na*NBPW - 2*NBPW;
	u.u_frame [FRAME_SP] = ap - 16;
        u.u_frame [FRAME_R4] = na - ne;			/* $a0 := argc */
        u.u_frame [FRAME_R5] = ap;			/* $a1 := argv */
        u.u_frame [FRAME_R6] = ap + (na-ne+1)*NBPW;	/* $a2 := env */
	*(int*) (USER_DATA_END - NBPW) = ap;            /* for /bin/ps */
	nc = 0;
	cc = 0;
	for (;;) {
		if (na == ne) {
			*(int*) ap = 0;
			ap += NBPW;
		}
		if (--na < 0)
			break;
		*(int*) ap = ucp;
		do {
			if (cc <= 0) {
				if (bp) {
					brelse(bp);
				}
				cc = DEV_BSIZE;
				bp = bread (swapdev, dbtofsb(bno) + lblkno(nc));
				bp->b_flags |= B_AGE;		/* throw away */
				bp->b_flags &= ~B_DELWRI;	/* cancel io */
				cp = bp->b_addr;
			}
			error = copystr (cp, (caddr_t) ucp, (unsigned) cc,
				&len);
//printf ("execve copy '%s' %u bytes from %08x to %08x\n", cp, len, cp, ucp);
			ucp += len;
			cp += len;
			nc += len;
			cc -= len;
		} while (error == ENOENT);
		if (error == EFAULT)
			panic ("exec: EFAULT");
		ap += NBPW;
	}
	*(int*) ap = 0;
	if (bp) {
		bp->b_flags |= B_AGE;
		brelse (bp);
		bp = NULL;
	}
	execsigs (u.u_procp);
	for (cp = u.u_pofile, cc = 0; cc <= u.u_lastfile; cc++, cp++) {
		if (*cp & UF_EXCLOSE) {
			(void) closef (u.u_ofile [cc]);
			u.u_ofile [cc] = NULL;
			*cp = 0;
		}
	}
	while (u.u_lastfile >= 0 && u.u_ofile [u.u_lastfile] == NULL)
		u.u_lastfile--;

	/*
	 * Clear registers.
	 */
        u.u_frame [FRAME_R1] = 0;			/* $at */
        u.u_frame [FRAME_R2] = 0;			/* $v0 */
        u.u_frame [FRAME_R3] = 0;			/* $v1 */
        u.u_frame [FRAME_R7] = 0;			/* $a3 */
        u.u_frame [FRAME_R8] = 0;			/* $t0 */
        u.u_frame [FRAME_R9] = 0;			/* $t1 */
        u.u_frame [FRAME_R10] = 0;			/* $t2 */
        u.u_frame [FRAME_R11] = 0;			/* $t3 */
        u.u_frame [FRAME_R12] = 0;			/* $t4 */
        u.u_frame [FRAME_R13] = 0;			/* $t5 */
        u.u_frame [FRAME_R14] = 0;			/* $t6 */
        u.u_frame [FRAME_R15] = 0;			/* $t7 */
        u.u_frame [FRAME_R16] = 0;			/* $s0 */
        u.u_frame [FRAME_R17] = 0;			/* $s1 */
        u.u_frame [FRAME_R18] = 0;			/* $s2 */
        u.u_frame [FRAME_R19] = 0;			/* $s3 */
        u.u_frame [FRAME_R20] = 0;			/* $s4 */
        u.u_frame [FRAME_R21] = 0;			/* $s5 */
        u.u_frame [FRAME_R22] = 0;			/* $s6 */
        u.u_frame [FRAME_R23] = 0;			/* $s7 */
        u.u_frame [FRAME_R24] = 0;			/* $t8 */
        u.u_frame [FRAME_R25] = 0;			/* $t9 */
        u.u_frame [FRAME_FP] = 0;
        u.u_frame [FRAME_RA] = 0;
        u.u_frame [FRAME_LO] = 0;
        u.u_frame [FRAME_HI] = 0;
        u.u_frame [FRAME_GP] = 0;
	u.u_frame [FRAME_PC] = exdata.ex_exec.a_entry;

	/*
	 * Remember file name for accounting.
	 */
	bcopy ((caddr_t) (indir ? cfname : ndp->ni_dent.d_name),
                (caddr_t) u.u_comm, MAXCOMLEN);
done:
//printf ("execve done: PC=%08x, SP=%08x, R4=%08x, R5=%08x, R6=%08x\n",
//    u.u_frame [FRAME_PC], u.u_frame [FRAME_SP],
//    u.u_frame [FRAME_R4], u.u_frame [FRAME_R5], u.u_frame [FRAME_R6]);
	if (bp) {
		bp->b_flags |= B_AGE;
		brelse (bp);
	}
	if (bno)
		mfree (swapmap, btod (NCARGS + MAXBSIZE), bno);
	if (ip)
		iput(ip);
}
