/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "systm.h"
#include "user.h"
#include "inode.h"
#include "namei.h"
#include "fs.h"
#include "file.h"
#include "stat.h"
#include "kernel.h"
#include "proc.h"

/*
 * Common routine for chroot and chdir.
 */
static void
chdirec(ipp)
	register struct inode **ipp;
{
	register struct inode *ip;
	struct a {
		char	*fname;
	} *uap = (struct a *)u.u_arg;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;

	NDINIT (ndp, LOOKUP, FOLLOW, uap->fname);
	ip = namei(ndp);
	if (ip == NULL)
		return;
	if ((ip->i_mode&IFMT) != IFDIR) {
		u.u_error = ENOTDIR;
		goto bad;
	}
	if (access(ip, IEXEC))
		goto bad;
	iunlock(ip);
	if (*ipp)
		irele(*ipp);
	*ipp = ip;
	return;

bad:
	iput(ip);
}

/*
 * Change current working directory (``.'').
 */
void
chdir()
{
	chdirec (&u.u_cdir);
}

void
fchdir()
{
	register struct	a {
		int	fd;
	} *uap = (struct a *)u.u_arg;
	register struct	inode *ip;

	ip = getinode (uap->fd);
	if (ip == NULL)
		return;
	ilock(ip);
	if ((ip->i_mode & IFMT) != IFDIR) {
		u.u_error = ENOTDIR;
		goto bad;
	}
	if (access (ip, IEXEC))
		goto bad;
	iunlock(ip);
	ip->i_count++;
	irele(u.u_cdir);
	u.u_cdir = ip;
	return;
bad:
	iunlock(ip);
	return;
}

/*
 * Change notion of root (``/'') directory.
 */
void
chroot()
{
	if (suser())
		chdirec (&u.u_rdir);
}

/*
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
static int
copen (mode, cmode, fname)
	int mode;
	int cmode;
	caddr_t fname;
{
	register struct inode *ip;
	register struct file *fp;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;
	int indx, type, flags, error;

        if (! fname)
                fname = "";
//printf ("copen (mode=%#o, cmode=%#o, fname=%#x '%s')\n", mode, cmode, fname, fname);
	fp = falloc();
	if (fp == NULL) {
//printf ("copen: falloc failed, errno=%d\n", u.u_error);
		return(u.u_error);	/* XXX */
        }
	flags = FFLAGS(mode);	/* convert from open to kernel flags */
	fp->f_flag = flags & FMASK;
	fp->f_type = DTYPE_INODE;
	cmode &= 077777 & ~ISVTX;
	indx = u.u_rval;
	u.u_dupfd = -indx - 1;
	NDINIT (ndp, LOOKUP, FOLLOW, fname);

	/*
	 * ENODEV is returned by the 'fdopen()' routine - see the comments in that
	 * routine for details about the hack being used.
	 *
	 * ENXIO only comes out of the 'portal fs' code (which 2.11BSD does not have).
	 * It probably should have been removed during the port of the 'file descriptor
	 * driver' since it's a "can not happen" event.
	 *
	 * u.u_dupfd is used because there the space in the proc structure is at a
	 * premium in 2.11 while space in the u structure is relatively free.  Also
	 * there were more unused (pad) fields available in 'u' as compared to 'proc'.
	 */
	error = vn_open(ndp, flags, cmode);
	if (error) {
//printf ("copen: vn_open failed, errno=%d\n", error);
		fp->f_count = 0;
		if ((error == ENODEV || error == ENXIO) &&
		    u.u_dupfd >= 0 &&
		    (error = dupfdopen(indx,u.u_dupfd,flags,error) == 0)) {
			u.u_rval = indx;
			return(0);
		}
		u.u_ofile[indx] = NULL;
		return(error);
	}
	ip = ndp->ni_ip;
	u.u_dupfd = 0;

	fp->f_data = (caddr_t)ip;

	if (flags & (O_EXLOCK | O_SHLOCK)) {
		if (flags & O_EXLOCK)
			type = LOCK_EX;
		else
			type = LOCK_SH;
		if (flags & FNONBLOCK)
			type |= LOCK_NB;
		error = ino_lock(fp, type);
		if (error) {
//printf ("copen: ino_lock failed, errno=%d\n", error);
			closef(fp);
			u.u_ofile[indx] = NULL;
		}
	}
//printf ("copen returned errno=%d\n", error);
	return(error);
}

/*
 * Open system call.
 */
void
open()
{
	register struct a {
		char	*fname;
		int	mode;
		int	crtmode;
	} *uap = (struct a *) u.u_arg;

	u.u_error = copen (uap->mode, uap->crtmode, uap->fname);
}

/*
 * Mknod system call
 */
void
mknod()
{
	register struct inode *ip;
	register struct a {
		char	*fname;
		int	fmode;
		int	dev;
	} *uap = (struct a *)u.u_arg;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;

	if (! suser())
		return;
	NDINIT (ndp, CREATE, NOFOLLOW, uap->fname);
	ip = namei(ndp);
	if (ip != NULL) {
		u.u_error = EEXIST;
		goto out;
	}
	if (u.u_error)
		return;
	ip = maknode (uap->fmode, ndp);
	if (ip == NULL)
		return;
	switch (ip->i_mode & IFMT) {

	case IFMT:	/* used by badsect to flag bad sectors */
	case IFCHR:
	case IFBLK:
		if (uap->dev) {
			/*
			 * Want to be able to use this to make badblock
			 * inodes, so don't truncate the dev number.
			 */
			ip->i_rdev = uap->dev;
			ip->i_dummy = 0;
			ip->i_flag |= IACC|IUPD|ICHG;
		}
	}
out:
	iput(ip);
}

/*
 * link system call
 */
void
link()
{
	register struct inode *ip, *xp;
	register struct a {
		char	*target;
		char	*linkname;
	} *uap = (struct a *)u.u_arg;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;

	NDINIT (ndp, LOOKUP, FOLLOW, uap->target);
	ip = namei(ndp);	/* well, this routine is doomed anyhow */
	if (ip == NULL)
		return;
	if ((ip->i_mode&IFMT) == IFDIR && !suser()) {
		iput(ip);
		return;
	}
	if (ip->i_flags & (IMMUTABLE|APPEND)) {
		u.u_error = EPERM;
		iput(ip);
		return;
	}
	ip->i_nlink++;
	ip->i_flag |= ICHG;
	iupdat(ip, &time, &time, 1);
	iunlock(ip);
	ndp->ni_nameiop = CREATE;
	ndp->ni_dirp = (caddr_t)uap->linkname;
	xp = namei(ndp);
	if (xp != NULL) {
		u.u_error = EEXIST;
		iput(xp);
		goto out;
	}
	if (u.u_error)
		goto out;
	if (ndp->ni_pdir->i_dev != ip->i_dev) {
		iput(ndp->ni_pdir);
		u.u_error = EXDEV;
		goto out;
	}
	u.u_error = direnter(ip, ndp);
out:
	if (u.u_error) {
		ip->i_nlink--;
		ip->i_flag |= ICHG;
	}
	irele(ip);
}

/*
 * symlink -- make a symbolic link
 */
void
symlink()
{
	register struct a {
		char	*target;
		char	*linkname;
	} *uap = (struct a *)u.u_arg;
	register struct inode *ip;
	char *tp;
	int c, nc;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;

	tp = uap->target;
	nc = 0;
	for (;;) {
		if (baduaddr (tp)) {
			u.u_error = EFAULT;
			return;
		}
		c = (u_char) *tp;
		if (c == 0)
			break;
		tp++;
		nc++;
	}
	NDINIT (ndp, CREATE, NOFOLLOW, uap->linkname);
	ip = namei(ndp);
	if (ip) {
		iput(ip);
		u.u_error = EEXIST;
		return;
	}
	if (u.u_error)
		return;
	ip = maknode (IFLNK | 0777, ndp);
	if (ip == NULL)
		return;
	u.u_error = rdwri (UIO_WRITE, ip, uap->target, nc, (off_t) 0,
				IO_UNIT, (int*) 0);
	/* handle u.u_error != 0 */
	iput(ip);
}

/*
 * Unlink system call.
 * Hard to avoid races here, especially
 * in unlinking directories.
 */
void
unlink()
{
	register struct a {
		char	*fname;
	} *uap = (struct a *)u.u_arg;
	register struct inode *ip, *dp;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;

	NDINIT (ndp, DELETE, LOCKPARENT, uap->fname);
	ip = namei(ndp);
	if (ip == NULL)
		return;
	dp = ndp->ni_pdir;
	if ((ip->i_mode&IFMT) == IFDIR && !suser())
		goto out;
	/*
	 * Don't unlink a mounted file.
	 */
	if (ip->i_dev != dp->i_dev) {
		u.u_error = EBUSY;
		goto out;
	}
	if ((ip->i_flags & (IMMUTABLE|APPEND)) || (dp->i_flags & APPEND)) {
		u.u_error = EPERM;
		goto out;
	}
	if (dirremove(ndp)) {
		ip->i_nlink--;
		ip->i_flag |= ICHG;
	}
out:
	if (dp == ip)
		irele(ip);
	else
		iput(ip);
	iput(dp);
}

/*
 * Access system call
 */
void
saccess()
{
	uid_t t_uid;
	gid_t t_gid;
	register struct inode *ip;
	register struct a {
		char	*fname;
		int	fmode;
	} *uap = (struct a *)u.u_arg;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;

	t_uid = u.u_uid;
	t_gid = u.u_groups[0];
	u.u_uid = u.u_ruid;
	u.u_groups[0] = u.u_rgid;
	NDINIT (ndp, LOOKUP, FOLLOW, uap->fname);
	ip = namei(ndp);
	if (ip != NULL) {
		if ((uap->fmode&R_OK) && access(ip, IREAD))
			goto done;
		if ((uap->fmode&W_OK) && access(ip, IWRITE))
			goto done;
		if ((uap->fmode&X_OK) && access(ip, IEXEC))
			goto done;
done:
		iput(ip);
	}
	u.u_uid = t_uid;
	u.u_groups[0] = t_gid;
}

static void
stat1 (follow)
	int follow;
{
	register struct inode *ip;
	register struct a {
		char	*fname;
		struct stat *ub;
	} *uap = (struct a *)u.u_arg;
	struct stat sb;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;

	NDINIT (ndp, LOOKUP, follow, uap->fname);
	ip = namei(ndp);
	if (ip == NULL)
		return;
	(void) ino_stat(ip, &sb);
	iput(ip);
	u.u_error = copyout((caddr_t)&sb, (caddr_t)uap->ub, sizeof (sb));
}

/*
 * Stat system call.  This version follows links.
 */
void
stat()
{
	stat1 (FOLLOW);
}

/*
 * Lstat system call.  This version does not follow links.
 */
void
lstat()
{
	stat1 (NOFOLLOW);
}

/*
 * Return target name of a symbolic link
 */
void
readlink()
{
	register struct inode *ip;
	register struct a {
		char	*name;
		char	*buf;
		int	count;
	} *uap = (struct a *)u.u_arg;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;
	int resid;

	NDINIT (ndp, LOOKUP, NOFOLLOW, uap->name);
	ip = namei(ndp);
	if (ip == NULL)
		return;
	if ((ip->i_mode&IFMT) != IFLNK) {
		u.u_error = EINVAL;
		goto out;
	}
	u.u_error = rdwri (UIO_READ, ip, uap->buf, uap->count, (off_t) 0,
				IO_UNIT, &resid);
out:
	iput(ip);
	u.u_rval = uap->count - resid;
}

static int
chflags1 (ip, flags)
	register struct inode *ip;
	u_short flags;
{
	struct	vattr	vattr;

	VATTR_NULL(&vattr);
	vattr.va_flags = flags;
	return(ufs_setattr(ip, &vattr));
}

/*
 * change flags of a file given pathname.
 */
void
chflags()
{
	register struct inode *ip;
	register struct a {
		char	*fname;
		u_int	flags;
	} *uap = (struct a *)u.u_arg;
	struct	nameidata nd;
	register struct nameidata *ndp = &nd;

	NDINIT (ndp, LOOKUP, FOLLOW, uap->fname);
	if ((ip = namei(ndp)) == NULL)
		return;
	u.u_error = chflags1 (ip, uap->flags);
	iput(ip);
}

/*
 * change flags of a file given file descriptor.
 */
void
fchflags()
{
	register struct a {
		int	fd;
		u_int	flags;
	} *uap = (struct a *)u.u_arg;
	register struct inode *ip;

	ip = getinode (uap->fd);
	if (ip == NULL)
		return;
	ilock(ip);
	u.u_error = chflags1 (ip, uap->flags);
	iunlock(ip);
}

/*
 * Change mode of a file given path name.
 */
void
chmod()
{
	register struct inode *ip;
	register struct a {
		char	*fname;
		int	fmode;
	} *uap = (struct a *)u.u_arg;
	struct	vattr	vattr;
	struct	nameidata nd;
	register struct nameidata *ndp = &nd;

	NDINIT (ndp, LOOKUP, FOLLOW, uap->fname);
	ip = namei(ndp);
	if (!ip)
		return;
	VATTR_NULL(&vattr);
	vattr.va_mode = uap->fmode & 07777;
	u.u_error = ufs_setattr(ip, &vattr);
	iput(ip);
}

/*
 * Change mode of a file given a file descriptor.
 */
void
fchmod()
{
	register struct a {
		int	fd;
		int	fmode;
	} *uap = (struct a *)u.u_arg;
	register struct inode *ip;
	struct	vattr	vattr;

	if ((ip = getinode(uap->fd)) == NULL)
		return;
	ilock(ip);
	VATTR_NULL(&vattr);
	vattr.va_mode = uap->fmode & 07777;
	u.u_error = ufs_setattr(ip, &vattr);
	iunlock(ip);
}

/*
 * Change the mode on a file.  This routine is called from ufs_setattr.
 * Inode must be locked before calling.
 */
int
chmod1(ip, mode)
	register struct inode *ip;
	register int mode;
{

	if (u.u_uid != ip->i_uid && !suser())
		return(u.u_error);
	if (u.u_uid) {
		if ((ip->i_mode & IFMT) != IFDIR && (mode & ISVTX))
			return(EFTYPE);
		if (!groupmember(ip->i_gid) && (mode & ISGID))
			return(EPERM);
	}
	ip->i_mode &= ~07777;		/* why? */
	ip->i_mode |= mode&07777;
	ip->i_flag |= ICHG;
	return (0);
}

/*
 * Set ownership given a path name.
 */
void
chown()
{
	register struct inode *ip;
	register struct a {
		char	*fname;
		int	uid;
		int	gid;
	} *uap = (struct a *)u.u_arg;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;
	struct	vattr	vattr;

	NDINIT (ndp, LOOKUP, NOFOLLOW, uap->fname);
	ip = namei(ndp);
	if (ip == NULL)
		return;
	VATTR_NULL(&vattr);
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	u.u_error = ufs_setattr(ip, &vattr);
	iput(ip);
}

/*
 * Set ownership given a file descriptor.
 */
void
fchown()
{
	register struct a {
		int	fd;
		int	uid;
		int	gid;
	} *uap = (struct a *)u.u_arg;
	register struct inode *ip;
	struct	vattr	vattr;

	if ((ip = getinode(uap->fd)) == NULL)
		return;
	ilock(ip);
	VATTR_NULL(&vattr);
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	u.u_error = ufs_setattr(ip, &vattr);
	iunlock(ip);
}

/*
 * Perform chown operation on inode ip.  This routine called from ufs_setattr.
 * inode must be locked prior to call.
 */
int
chown1 (ip, uid, gid)
	register struct inode *ip;
	register int uid, gid;
{
	int ouid, ogid;

	if (uid == -1)
		uid = ip->i_uid;
	if (gid == -1)
		gid = ip->i_gid;
	/*
	 * If we don't own the file, are trying to change the owner
	 * of the file, or are not a member of the target group,
	 * the caller must be superuser or the call fails.
	 */
	if ((u.u_uid != ip->i_uid || uid != ip->i_uid ||
	    !groupmember((gid_t)gid)) && !suser())
		return (u.u_error);
	ouid = ip->i_uid;
	ogid = ip->i_gid;
	ip->i_uid = uid;
	ip->i_gid = gid;
	if (ouid != uid || ogid != gid)
		ip->i_flag |= ICHG;
	if (ouid != uid && u.u_uid != 0)
		ip->i_mode &= ~ISUID;
	if (ogid != gid && u.u_uid != 0)
		ip->i_mode &= ~ISGID;
	return (0);
}

/*
 * Truncate a file given its path name.
 */
void
truncate()
{
	register struct a {
		char	*fname;
		off_t	length;
	} *uap = (struct a *)u.u_arg;
	register struct inode *ip;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;
	struct	vattr	vattr;

	NDINIT (ndp, LOOKUP, FOLLOW, uap->fname);
	ip = namei(ndp);
	if (ip == NULL)
		return;
	if (access(ip, IWRITE))
		goto bad;
	VATTR_NULL(&vattr);
	vattr.va_size = uap->length;
	u.u_error = ufs_setattr(ip, &vattr);
bad:
	iput(ip);
}

/*
 * Truncate a file given a file descriptor.
 */
void
ftruncate()
{
	register struct a {
		int	fd;
		off_t	length;
	} *uap = (struct a *)u.u_arg;
	register struct inode *ip;
	register struct file *fp;
	struct	vattr	vattr;

	if ((fp = getf(uap->fd)) == NULL)
		return;
	if (!(fp->f_flag&FWRITE) || (fp->f_type != DTYPE_INODE)) {
		u.u_error = EINVAL;
		return;
	}
	ip = (struct inode *)fp->f_data;
	ilock(ip);
	VATTR_NULL(&vattr);
	vattr.va_size = uap->length;
	u.u_error = ufs_setattr(ip, &vattr);
	iunlock(ip);
}

/*
 * Rename system call.
 * 	rename("foo", "bar");
 * is essentially
 *	unlink("bar");
 *	link("foo", "bar");
 *	unlink("foo");
 * but ``atomically''.  Can't do full commit without saving state in the
 * inode on disk which isn't feasible at this time.  Best we can do is
 * always guarantee the target exists.
 *
 * Basic algorithm is:
 *
 * 1) Bump link count on source while we're linking it to the
 *    target.  This also insure the inode won't be deleted out
 *    from underneath us while we work (it may be truncated by
 *    a concurrent `trunc' or `open' for creation).
 * 2) Link source to destination.  If destination already exists,
 *    delete it first.
 * 3) Unlink source reference to inode if still around. If a
 *    directory was moved and the parent of the destination
 *    is different from the source, patch the ".." entry in the
 *    directory.
 *
 * Source and destination must either both be directories, or both
 * not be directories.  If target is a directory, it must be empty.
 */
void
rename()
{
	struct a {
		char	*from;
		char	*to;
	} *uap = (struct a *)u.u_arg;
	register struct inode *ip, *xp, *dp;
	struct dirtemplate dirbuf;
	int doingdirectory = 0, oldparent = 0, newparent = 0;
	struct	nameidata nd;
	register struct nameidata *ndp = &nd;
	int error = 0;

	NDINIT (ndp, DELETE, LOCKPARENT, uap->from);
	ip = namei(ndp);
	if (ip == NULL)
		return;
	dp = ndp->ni_pdir;
	/*
	 * 'from' file can not be renamed if it is immutable/appendonly or if its
	 * parent directory is append only.
	 */
	if ((ip->i_flags & (IMMUTABLE|APPEND)) || (dp->i_flags & APPEND)) {
		iput(dp);
		if (dp == ip)
			irele(ip);
		else
			iput(ip);
		u.u_error = EPERM;
		return;
	}

	if ((ip->i_mode&IFMT) == IFDIR) {
		register struct direct *d;

		d = &ndp->ni_dent;
		/*
		 * Avoid ".", "..", and aliases of "." for obvious reasons.
		 */
		if ((d->d_namlen == 1 && d->d_name[0] == '.') ||
		    (d->d_namlen == 2 && bcmp(d->d_name, "..", 2) == 0) ||
		    (dp == ip) || (ip->i_flag & IRENAME)) {
			iput(dp);
			if (dp == ip)
				irele(ip);
			else
				iput(ip);
			u.u_error = EINVAL;
			return;
		}
		ip->i_flag |= IRENAME;
		oldparent = dp->i_number;
		doingdirectory++;
	}
	iput(dp);

	/*
	 * 1) Bump link count while we're moving stuff
	 *    around.  If we crash somewhere before
	 *    completing our work, the link count
	 *    may be wrong, but correctable.
	 */
	ip->i_nlink++;
	ip->i_flag |= ICHG;
	iupdat(ip, &time, &time, 1);
	iunlock(ip);

	/*
	 * When the target exists, both the directory
	 * and target inodes are returned locked.
	 */
	ndp->ni_nameiop = CREATE | LOCKPARENT | NOCACHE;
	ndp->ni_dirp = (caddr_t)uap->to;
	xp = namei(ndp);
	if (u.u_error) {
		error = u.u_error;
		goto out;
	}
	dp = ndp->ni_pdir;
	/*
	 * rename can not be done if 'to' file exists and is immutable/appendonly
	 * or if the directory is append only (this is because an existing 'to'
	 * has to be deleted first and that is illegal in an appendonly directory).
	 */
	if (xp && ((xp->i_flags & (IMMUTABLE|APPEND)) || (dp->i_flags & APPEND))) {
		error = EPERM;
		goto bad;
	}

	/*
	 * If ".." must be changed (ie the directory gets a new
	 * parent) then the source directory must not be in the
	 * directory heirarchy above the target, as this would
	 * orphan everything below the source directory. Also
	 * the user must have write permission in the source so
	 * as to be able to change "..". We must repeat the call
	 * to namei, as the parent directory is unlocked by the
	 * call to checkpath().
	 */
	if (oldparent != dp->i_number)
		newparent = dp->i_number;
	if (doingdirectory && newparent) {
		if (access(ip, IWRITE))
			goto bad;
		do {
			dp = ndp->ni_pdir;
			if (xp != NULL)
				iput(xp);
			u.u_error = checkpath(ip, dp);
			if (u.u_error)
				goto out;
			xp = namei(ndp);
			if (u.u_error) {
				error = u.u_error;
				goto out;
			}
		} while (dp != ndp->ni_pdir);
	}
	/*
	 * 2) If target doesn't exist, link the target
	 *    to the source and unlink the source.
	 *    Otherwise, rewrite the target directory
	 *    entry to reference the source inode and
	 *    expunge the original entry's existence.
	 */
	if (xp == NULL) {
		if (dp->i_dev != ip->i_dev) {
			error = EXDEV;
			goto bad;
		}
		/*
		 * Account for ".." in new directory.
		 * When source and destination have the same
		 * parent we don't fool with the link count.
		 */
		if (doingdirectory && newparent) {
			dp->i_nlink++;
			dp->i_flag |= ICHG;
			iupdat(dp, &time, &time, 1);
		}
		error = direnter(ip, ndp);
		if (error)
			goto out;
	} else {
		if (xp->i_dev != dp->i_dev || xp->i_dev != ip->i_dev) {
			error = EXDEV;
			goto bad;
		}
		/*
		 * Short circuit rename(foo, foo).
		 */
		if (xp->i_number == ip->i_number)
			goto bad;
		/*
		 * If the parent directory is "sticky", then the user must
		 * own the parent directory, or the destination of the rename,
		 * otherwise the destination may not be changed (except by
		 * root). This implements append-only directories.
		 */
		if ((dp->i_mode & ISVTX) && u.u_uid != 0 &&
		    u.u_uid != dp->i_uid && xp->i_uid != u.u_uid) {
			error = EPERM;
			goto bad;
		}
		/*
		 * Target must be empty if a directory
		 * and have no links to it.
		 * Also, insure source and target are
		 * compatible (both directories, or both
		 * not directories).
		 */
		if ((xp->i_mode&IFMT) == IFDIR) {
			if (!dirempty(xp, dp->i_number) || xp->i_nlink > 2) {
				error = ENOTEMPTY;
				goto bad;
			}
			if (!doingdirectory) {
				error = ENOTDIR;
				goto bad;
			}
			cacheinval(dp);
		} else if (doingdirectory) {
			error = EISDIR;
			goto bad;
		}
		dirrewrite(dp, ip, ndp);
		if (u.u_error) {
			error = u.u_error;
			goto bad1;
		}
		/*
		 * Adjust the link count of the target to
		 * reflect the dirrewrite above.  If this is
		 * a directory it is empty and there are
		 * no links to it, so we can squash the inode and
		 * any space associated with it.  We disallowed
		 * renaming over top of a directory with links to
		 * it above, as the remaining link would point to
		 * a directory without "." or ".." entries.
		 */
		xp->i_nlink--;
		if (doingdirectory) {
			if (--xp->i_nlink != 0)
				panic("rename: lnk dir");
			itrunc(xp, (u_long)0, 0);	/* IO_SYNC? */
		}
		xp->i_flag |= ICHG;
		iput(xp);
		xp = NULL;
	}

	/*
	 * 3) Unlink the source.
	 */
	NDINIT (ndp, DELETE, LOCKPARENT, uap->from);
	xp = namei(ndp);
	if (xp != NULL)
		dp = ndp->ni_pdir;
	else
		dp = NULL;
	/*
	 * Insure that the directory entry still exists and has not
	 * changed while the new name has been entered. If the source is
	 * a file then the entry may have been unlinked or renamed. In
	 * either case there is no further work to be done. If the source
	 * is a directory then it cannot have been rmdir'ed; its link
	 * count of three would cause a rmdir to fail with ENOTEMPTY.
	 * The IRENAME flag insures that it cannot be moved by another
	 * rename.
	 */
	if (xp != ip) {
		if (doingdirectory)
			panic("rename: lost dir entry");
	} else {
		/*
		 * If the source is a directory with a
		 * new parent, the link count of the old
		 * parent directory must be decremented
		 * and ".." set to point to the new parent.
		 */
		if (doingdirectory && newparent) {
			dp->i_nlink--;
			dp->i_flag |= ICHG;
			error = rdwri (UIO_READ, xp, (caddr_t) &dirbuf,
					sizeof(struct dirtemplate), (off_t) 0,
					IO_UNIT, (int*) 0);

			if (error == 0) {
				if (dirbuf.dotdot_namlen != 2 ||
				    dirbuf.dotdot_name[0] != '.' ||
				    dirbuf.dotdot_name[1] != '.') {
					printf("rename: mangled dir\n");
				} else {
					dirbuf.dotdot_ino = newparent;
					(void) rdwri (UIO_WRITE, xp,
						(caddr_t) &dirbuf,
						sizeof(struct dirtemplate),
						(off_t) 0,
						IO_UNIT | IO_SYNC, (int*) 0);
					cacheinval(dp);
				}
			}
		}
		if (dirremove(ndp)) {
			xp->i_nlink--;
			xp->i_flag |= ICHG;
		}
		xp->i_flag &= ~IRENAME;
		if (error == 0)		/* XXX conservative */
			error = u.u_error;
	}
	if (dp)
		iput(dp);
	if (xp)
		iput(xp);
	irele(ip);
	if (error)
		u.u_error = error;
	return;

bad:
	iput(dp);
bad1:
	if (xp)
		iput(xp);
out:
	ip->i_nlink--;
	ip->i_flag |= ICHG;
	irele(ip);
	if (error)
		u.u_error = error;
}

/*
 * Make a new file.
 */
struct inode *
maknode (mode, ndp)
	int mode;
	register struct nameidata *ndp;
{
	register struct inode *ip;
	register struct inode *pdir = ndp->ni_pdir;

	ip = ialloc(pdir);
	if (ip == NULL) {
		iput(pdir);
		return (NULL);
	}
	ip->i_flag |= IACC|IUPD|ICHG;
	if ((mode & IFMT) == 0)
		mode |= IFREG;
	ip->i_mode = mode & ~u.u_cmask;
	ip->i_nlink = 1;
	ip->i_uid = u.u_uid;
	ip->i_gid = pdir->i_gid;
	if (ip->i_mode & ISGID && !groupmember(ip->i_gid))
		ip->i_mode &= ~ISGID;
	/*
	 * Make sure inode goes to disk before directory entry.
	 */
	iupdat(ip, &time, &time, 1);
	u.u_error = direnter(ip, ndp);
	if (u.u_error) {
		/*
		 * Write error occurred trying to update directory
		 * so must deallocate the inode.
		 */
		ip->i_nlink = 0;
		ip->i_flag |= ICHG;
		iput(ip);
		return (NULL);
	}
	ndp->ni_ip = ip;
	return (ip);
}

/*
 * A virgin directory (no blushing please).
 */
const struct dirtemplate mastertemplate = {
	0, 12,              1, ".",
	0, DIRBLKSIZ - 12,  2, "..",
};

/*
 * Mkdir system call
 */
void
mkdir()
{
	register struct a {
		char	*name;
		int	dmode;
	} *uap = (struct a *)u.u_arg;
	register struct inode *ip, *dp;
	struct dirtemplate dirtemplate;
	struct	nameidata nd;
	register struct nameidata *ndp = &nd;

	NDINIT (ndp, CREATE, NOFOLLOW, uap->name);
	ip = namei(ndp);
	if (u.u_error)
		return;
	if (ip != NULL) {
		iput(ip);
		u.u_error = EEXIST;
		return;
	}
	dp = ndp->ni_pdir;
	uap->dmode &= 0777;
	uap->dmode |= IFDIR;
	/*
	 * Must simulate part of maknode here
	 * in order to acquire the inode, but
	 * not have it entered in the parent
	 * directory.  The entry is made later
	 * after writing "." and ".." entries out.
	 */
	ip = ialloc(dp);
	if (ip == NULL) {
		iput(dp);
		return;
	}
	ip->i_flag |= IACC|IUPD|ICHG;
	ip->i_mode = uap->dmode & ~u.u_cmask;
	ip->i_nlink = 2;
	ip->i_uid = u.u_uid;
	ip->i_gid = dp->i_gid;
	iupdat(ip, &time, &time, 1);

	/*
	 * Bump link count in parent directory
	 * to reflect work done below.  Should
	 * be done before reference is created
	 * so reparation is possible if we crash.
	 */
	dp->i_nlink++;
	dp->i_flag |= ICHG;
	iupdat(dp, &time, &time, 1);

	/*
	 * Initialize directory with "."
	 * and ".." from static template.
	 */
	dirtemplate = mastertemplate;
	dirtemplate.dot_ino = ip->i_number;
	dirtemplate.dotdot_ino = dp->i_number;
	u.u_error = rdwri (UIO_WRITE, ip, (caddr_t) &dirtemplate,
		sizeof (dirtemplate), (off_t) 0, IO_UNIT | IO_SYNC, (int*) 0);
	if (u.u_error) {
		dp->i_nlink--;
		dp->i_flag |= ICHG;
		goto bad;
	}
	ip->i_size = DIRBLKSIZ;
	/*
	 * Directory all set up, now
	 * install the entry for it in
	 * the parent directory.
	 */
	u.u_error = direnter(ip, ndp);
	dp = NULL;
	if (u.u_error) {
		NDINIT (ndp, LOOKUP, NOCACHE, uap->name);
		dp = namei(ndp);
		if (dp) {
			dp->i_nlink--;
			dp->i_flag |= ICHG;
		}
	}
bad:
	/*
	 * No need to do an explicit itrunc here,
	 * irele will do this for us because we set
	 * the link count to 0.
	 */
	if (u.u_error) {
		ip->i_nlink = 0;
		ip->i_flag |= ICHG;
	}
	if (dp)
		iput(dp);
	iput(ip);
}

/*
 * Rmdir system call.
 */
void
rmdir()
{
	struct a {
		char	*name;
	} *uap = (struct a *)u.u_arg;
	register struct inode *ip, *dp;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;

	NDINIT (ndp, DELETE, LOCKPARENT, uap->name);
	ip = namei(ndp);
	if (ip == NULL)
		return;
	dp = ndp->ni_pdir;
	/*
	 * No rmdir "." please.
	 */
	if (dp == ip) {
		irele(dp);
		iput(ip);
		u.u_error = EINVAL;
		return;
	}
	if ((ip->i_mode&IFMT) != IFDIR) {
		u.u_error = ENOTDIR;
		goto out;
	}
	/*
	 * Don't remove a mounted on directory.
	 */
	if (ip->i_dev != dp->i_dev) {
		u.u_error = EBUSY;
		goto out;
	}
	/*
	 * Verify the directory is empty (and valid).
	 * (Rmdir ".." won't be valid since
	 *  ".." will contain a reference to
	 *  the current directory and thus be
	 *  non-empty.)
	 */
	if (ip->i_nlink != 2 || !dirempty(ip, dp->i_number)) {
		u.u_error = ENOTEMPTY;
		goto out;
	}
	if ((dp->i_flags & APPEND) || (ip->i_flags & (IMMUTABLE|APPEND))) {
		u.u_error = EPERM;
		goto out;
	}
	/*
	 * Delete reference to directory before purging
	 * inode.  If we crash in between, the directory
	 * will be reattached to lost+found,
	 */
	if (dirremove(ndp) == 0)
		goto out;
	dp->i_nlink--;
	dp->i_flag |= ICHG;
	cacheinval(dp);
	iput(dp);
	dp = NULL;
	/*
	 * Truncate inode.  The only stuff left
	 * in the directory is "." and "..".  The
	 * "." reference is inconsequential since
	 * we're quashing it.  The ".." reference
	 * has already been adjusted above.  We've
	 * removed the "." reference and the reference
	 * in the parent directory, but there may be
	 * other hard links so decrement by 2 and
	 * worry about them later.
	 */
	ip->i_nlink -= 2;
	itrunc(ip, (u_long)0, 0);	/* IO_SYNC? */
	cacheinval(ip);
out:
	if (dp)
		iput(dp);
	iput(ip);
}

/*
 * Get an inode pointer of a file descriptor.
 */
struct inode *
getinode(fdes)
	int fdes;
{
	register struct file *fp;

	if ((unsigned)fdes >= NOFILE || (fp = u.u_ofile[fdes]) == NULL) {
		u.u_error = EBADF;
		return ((struct inode *)0);
	}
	if (fp->f_type != DTYPE_INODE) {
		u.u_error = EINVAL;
		return ((struct inode *)0);
	}
	return((struct inode *)fp->f_data);
}
