/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
#include <sys/file.h>
#include <sys/user.h>
#include <sys/namei.h>
#include <sys/inode.h>
#include <sys/stat.h>

#include "systm.h"

/*
 * 2.11BSD does not have "vnodes", having instead only old fashioned "inodes".
 * The routine names (i.e. vn_open) were retained since the functions them-
 * selves were ported over with minimal change.  Retaining the 4.4 function
 * names also makes it easier to follow the logic flow when reading the 4.4
 * sources.  Also, changing the names from vn_* to in_* could have caused
 * confusion with the networking routines since 'in_' and 'ip_' are frequently
 * used in the networking code.
 *
 * The tab spacing has been altered to be (to me) more readable.
*/

/*
 * Common code for vnode open operations.
 * Check permissions, and call the VOP_OPEN (openi for 2.11) or VOP_CREATE
 * (maknode) routine.
 */
int
vn_open (ndp, fmode, cmode)
	register struct nameidata *ndp;
	int fmode, cmode;
{
	register struct inode *ip;
	register int error;

	if (fmode & O_CREAT) {
		if ((fmode & O_EXCL) == 0)
			ndp->ni_nameiop |= (CREATE|FOLLOW);
		else
			ndp->ni_nameiop = CREATE;
		ip = namei(ndp);
		if (ip == NULL) {
			if (u.u_error) {
				goto retuerr;
                        }
			ip = maknode(cmode, ndp);
			if (ip == NULL) {
				goto retuerr;
                        }
			fmode &= ~O_TRUNC;
		} else {
			if (fmode & O_EXCL) {
				error = EEXIST;
				goto bad;
			}
			fmode &= ~O_CREAT;
		}
	} else {
		ndp->ni_nameiop = LOOKUP | FOLLOW;
		ip = namei(ndp);
		if (ip == NULL) {
			goto retuerr;
                }
	}
	if ((ip->i_mode & IFMT) == IFSOCK) {
		error = EOPNOTSUPP;
		goto bad;
	}
	if ((ip->i_flags & APPEND) && (fmode&(FWRITE|O_APPEND)) == FWRITE) {
		error = EPERM;
		goto bad;
	}
	if ((fmode & O_CREAT) == 0) {
		if (fmode & FREAD) {
			if (access(ip, IREAD)) {
				error = u.u_error;	/* XXX */
				goto bad;
			}
		}
		if (fmode & (FWRITE | O_TRUNC)) {
			if ((ip->i_mode & IFMT) == IFDIR) {
				error = EISDIR;
				goto bad;
			}
			if (access(ip, IWRITE)) {
				error = u.u_error;
				goto bad;
			}
		}
	}
	if (fmode & O_TRUNC)
		itrunc(ip, (off_t)0, fmode & O_FSYNC ? IO_SYNC : 0);
	/*
	 * 4.4 returns the vnode locked from vn_open which means that each caller
	 * has to go and unlock it.
	 *
	 * 2.11 returns the inode unlocked (for now).
	 */
	iunlock(ip);		/* because namei returns a locked inode */
	if (setjmp(&u.u_qsave)) {
		error = EINTR;	/* opens are not restarted after signals */
		goto lbad;
	}
	error = openi (ip, fmode);
	if (error) {
		goto lbad;
        }
	return(0);

	/*
	 * Gratuitous lock but it does (correctly) implement the earlier behaviour of
	 * copen (it also avoids a panic in iput).
	 */
lbad:
	ilock(ip);

bad:
	/*
	 * Do NOT do an 'ilock' here - this tag is to be used only when the inode is
	 * locked (i.e. from namei).
	 */
	iput(ip);
	return(error);

retuerr:
	return(u.u_error);	/* XXX - Bletch */
}

/*
 * Inode close call.  Pipes and sockets do NOT enter here.  This routine is
 * used by the kernel to close files it opened for itself.
 * The kernel does not create sockets or pipes on its own behalf.
 *
 * The difference between this routine and vn_closefile below is that vn_close
 * takes an "inode *" as a first argument and is passed the flags by the caller
 * while vn_closefile (called from the closef routine for DTYPE_INODE inodes)
 * takes a "file *" and extracts the flags from the file structure.
 */
int
vn_close(ip, flags)
	register struct inode *ip;
	int flags;
{
	register int error;

	error = closei(ip, flags);
	irele(ip);			/* assumes inode is unlocked */
	return(error);
}

/*
 * File table inode close routine.  This is called from 'closef()' via the
 * "Fops" table (the 'inodeops' entry).
 *
 * NOTE: pipes are a special case of inode and have their own 'pipe_close'
 * entry in the 'pipeops' table. See sys_pipe.c for pipe_close().
 *
 * In 4.4BSD this routine called vn_close() but since 2.11 does not do the
 * writecheck counting we can skip the overhead of nesting another level down
 * and call closei() and irele() ourself.
 */
int
vn_closefile(fp)
	register struct file *fp;
{
	register struct inode *ip = (struct inode *)fp->f_data;

	/*
	 * Need to clear the inode pointer in the file structure so that the
	 * inode is not seen during the scan for aliases of character or block
	 * devices in closei().
	 */
	fp->f_data = (caddr_t)0;	/* XXX */
	irele(ip);
	return (closei(ip, fp->f_flag));
}
