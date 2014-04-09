/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "systm.h"
#include "user.h"
#include "inode.h"
#include "fs.h"
#include "buf.h"
#include "mount.h"
#include "file.h"
#include "namei.h"
#include "conf.h"
#include "stat.h"
#include "ioctl.h"
#include "proc.h"

/*
 * Common code for mount and umount.
 * Check that the user's argument is a reasonable
 * thing on which to mount, otherwise return error.
 */
static int
getmdev (pdev, fname)
	caddr_t fname;
	dev_t *pdev;
{
	register dev_t dev;
	register struct inode *ip;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;

	if (!suser())
		return (u.u_error);
	NDINIT (ndp, LOOKUP, FOLLOW, fname);
	ip = namei(ndp);
	if (ip == NULL) {
		if (u.u_error == ENOENT)
			return (ENODEV); /* needs translation */
		return (u.u_error);
	}
	if ((ip->i_mode&IFMT) != IFBLK) {
		iput(ip);
		return (ENOTBLK);
	}
	dev = (dev_t)ip->i_rdev;
	iput(ip);
	if (major(dev) >= nblkdev)
		return (ENXIO);
	*pdev = dev;
	return (0);
}

void
mount_updname (fs, on, from, lenon, lenfrom)
	struct	fs	*fs;
	char	*on, *from;
	int	lenon, lenfrom;
{
	struct	mount	*mp;

	bzero (fs->fs_fsmnt, sizeof (fs->fs_fsmnt));
	bcopy (on, fs->fs_fsmnt, sizeof (fs->fs_fsmnt) - 1);
	mp = (struct mount*) ((int) fs - offsetof (struct mount, m_filsys));
	bzero (mp->m_mnton, sizeof (mp->m_mnton));
	bzero (mp->m_mntfrom, sizeof (mp->m_mntfrom));
	bcopy (on, mp->m_mnton, lenon);
	bcopy (from, mp->m_mntfrom, lenfrom);
}

void
smount()
{
	register struct a {
		char	*fspec;
		char	*freg;
		int	flags;
	} *uap = (struct a *)u.u_arg;
	dev_t dev = 0;
	register struct inode *ip;
	register struct fs *fs;
	struct	nameidata nd;
	struct	nameidata *ndp = &nd;
	struct	mount	*mp;
	u_int lenon, lenfrom;
	int	error = 0;
	char	mnton[MNAMELEN], mntfrom[MNAMELEN];

	u.u_error = getmdev (&dev, uap->fspec);
	if (u.u_error)
		return;
	NDINIT (ndp, LOOKUP, FOLLOW, uap->freg);
	if ((ip = namei(ndp)) == NULL)
		return;
	if ((ip->i_mode&IFMT) != IFDIR) {
		error = ENOTDIR;
		goto	cmnout;
	}
	copystr (uap->freg, mnton, sizeof (mnton) - 1, &lenon);
	copystr (uap->fspec, mntfrom, sizeof (mntfrom) - 1, &lenfrom);

	if (uap->flags & MNT_UPDATE) {
		fs = ip->i_fs;
		mp = (struct mount *)
			((int)fs - offsetof(struct mount, m_filsys));
		if (ip->i_number != ROOTINO) {
			error = EINVAL;		/* Not a mount point */
			goto cmnout;
		}
		/*
		 * Check that the device passed in is the same one that is in the mount
		 * table entry for this mount point.
		 */
		if (dev != mp->m_dev) {
			error = EINVAL;		/* not right mount point */
			goto cmnout;
		}
		/*
		 * This is where the RW to RO transformation would be done.  It is, for now,
		 * too much work to port pages of code to do (besides which most
		 * programs get very upset at having access yanked out from under them).
		 */
		if (fs->fs_ronly == 0 && (uap->flags & MNT_RDONLY)) {
			error = EPERM;		/* ! RW to RO updates */
			goto cmnout;
		}
		/*
		 * However, going from RO to RW is easy.  Then merge in the new
		 * flags (async, sync, nodev, etc) passed in from the program.
		 */
		if (fs->fs_ronly && ((uap->flags & MNT_RDONLY) == 0)) {
			fs->fs_ronly = 0;
			mp->m_flags &= ~MNT_RDONLY;
		}
#define	_MF (MNT_NOSUID | MNT_NODEV | MNT_NOEXEC | MNT_ASYNC | MNT_SYNCHRONOUS | MNT_NOATIME)
		mp->m_flags &= ~_MF;
		mp->m_flags |= (uap->flags & _MF);
#undef _MF
		iput(ip);
		u.u_error = 0;
		goto updname;
	} else {
		/*
		 * This is where a new mount (not an update of an existing mount point) is
		 * done.
		 *
		 * The directory being mounted on can have no other references AND can not
		 * currently be a mount point.  Mount points have an inode number of (you
		 * guessed it) ROOTINO which is 2.
		 */
		if (ip->i_count != 1 || (ip->i_number == ROOTINO)) {
			error = EBUSY;
			goto cmnout;
		}
		fs = mountfs (dev, uap->flags, ip);
		if (fs == 0)
			return;
	}
	/*
	 * Lastly, both for new mounts and updates of existing mounts, update the
	 * mounted-on and mounted-from fields.
	 */
updname:
	mount_updname(fs, mnton, mntfrom, lenon, lenfrom);
	return;
cmnout:
	iput(ip);
	u.u_error = error;
}

/*
 * Mount a filesystem on the given directory inode.
 *
 * this routine has races if running twice
 */
struct fs *
mountfs (dev, flags, ip)
	dev_t dev;
	int flags;
	struct inode *ip;
{
	register struct mount *mp = 0;
	struct buf *tp = 0;
	register struct fs *fs;
	register int error;
	int ronly = flags & MNT_RDONLY;
	int needclose = 0;

	error = (*bdevsw[major(dev)].d_open) (dev,
		ronly ? FREAD : (FREAD | FWRITE), S_IFBLK);
	if (error)
		goto out;

	needclose = 1;
	tp = bread (dev, SUPERB);
	if (tp->b_flags & B_ERROR)
		goto out;
	for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
		if (mp->m_inodp != 0 && dev == mp->m_dev) {
			mp = 0;
			error = EBUSY;
			needclose = 0;
			goto out;
		}
	for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
		if (mp->m_inodp == 0)
			goto found;
	mp = 0;
	error = EMFILE;		/* needs translation */
	goto out;
found:
	mp->m_inodp = ip;	/* reserve slot */
	mp->m_dev = dev;
	fs = &mp->m_filsys;
	bcopy (tp->b_addr, (caddr_t)fs, sizeof(struct fs));
	brelse (tp);
	tp = 0;
	if (fs->fs_magic1 != FSMAGIC1 || fs->fs_magic2 != FSMAGIC2) {
		error = EINVAL;
		goto out;
	}
	fs->fs_ronly = (ronly != 0);
	if (ronly == 0)
		fs->fs_fmod = 1;
	fs->fs_ilock = 0;
	fs->fs_flock = 0;
	fs->fs_nbehind = 0;
	fs->fs_lasti = 1;
	fs->fs_flags = flags;
	if (ip) {
		ip->i_flag |= IMOUNT;
		cacheinval(ip);
		IUNLOCK(ip);
	}
	return (fs);
out:
	if (error == 0)
		error = EIO;
	if (ip)
		iput(ip);
	if (mp)
		mp->m_inodp = 0;
	if (tp)
		brelse(tp);
	if (needclose) {
		(*bdevsw[major(dev)].d_close)(dev,
			ronly? FREAD : FREAD|FWRITE, S_IFBLK);
		binval(dev);
	}
	u.u_error = error;
	return (0);
}

static int
unmount1 (fname)
	caddr_t fname;
{
	dev_t dev = 0;
	register struct mount *mp;
	register struct inode *ip;
	register int error;
	int aflag;

	error = getmdev(&dev, fname);
	if (error)
		return (error);
	for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
		if (mp->m_inodp != NULL && dev == mp->m_dev)
			goto found;
	return (EINVAL);
found:
	nchinval (dev);	/* flush the name cache */
	aflag = mp->m_flags & MNT_ASYNC;
	mp->m_flags &= ~MNT_ASYNC;	/* Don't want async when unmounting */
	ufs_sync(mp);

	if (iflush(dev) < 0) {
		mp->m_flags |= aflag;
		return (EBUSY);
	}
	ip = mp->m_inodp;
	ip->i_flag &= ~IMOUNT;
	irele(ip);
	mp->m_inodp = 0;
	mp->m_dev = 0;
	(*bdevsw[major(dev)].d_close)(dev, 0, S_IFBLK);
	binval(dev);
	return (0);
}

void
umount()
{
	struct a {
		char	*fspec;
	} *uap = (struct a *)u.u_arg;

	u.u_error = unmount1 (uap->fspec);
}
