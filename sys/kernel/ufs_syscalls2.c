/*
 * ufs_syscalls was getting too large.  Various UFS related system calls were
 * relocated to this file.
 */
#include "param.h"
#include "sys/file.h"
#include "user.h"
#include "inode.h"
#include "buf.h"
#include "fs.h"
#include "namei.h"
#include "mount.h"
#include "kernel.h"
#include "systm.h"
#include "proc.h"

static int
statfs1 (mp, sbp)
	struct	mount	*mp;
	struct	statfs	*sbp;
{
	struct	statfs	sfs;
	register struct	statfs	*sfsp;
	struct	fs	*fs = &mp->m_filsys;

	sfsp = &sfs;
	sfsp->f_type = MOUNT_UFS;
	sfsp->f_bsize = MAXBSIZE;
	sfsp->f_iosize = MAXBSIZE;
	sfsp->f_blocks = fs->fs_fsize - fs->fs_isize;
	sfsp->f_bfree = fs->fs_tfree;
	sfsp->f_bavail = fs->fs_tfree;
	sfsp->f_files = (fs->fs_isize - 1) * INOPB;
	sfsp->f_ffree = fs->fs_tinode;

	bcopy (mp->m_mnton, sfsp->f_mntonname, MNAMELEN);
	bcopy (mp->m_mntfrom, sfsp->f_mntfromname, MNAMELEN);
	sfsp->f_flags = mp->m_flags & MNT_VISFLAGMASK;
	return copyout ((caddr_t) sfsp, (caddr_t) sbp, sizeof (struct statfs));
}

void
statfs()
{
	register struct a {
		char	*path;
		struct	statfs	*buf;
	} *uap = (struct a *)u.u_arg;
	register struct	inode	*ip;
	struct	nameidata nd;
	register struct nameidata *ndp = &nd;
	struct	mount	*mp;

	NDINIT (ndp, LOOKUP, FOLLOW, uap->path);
	ip = namei(ndp);
	if (! ip)
		return;
	mp = (struct mount *)((int)ip->i_fs - offsetof(struct mount, m_filsys));
	iput(ip);
	u.u_error = statfs1 (mp, uap->buf);
}

void
fstatfs()
{
	register struct a {
		int	fd;
		struct	statfs *buf;
	} *uap = (struct a *)u.u_arg;
	register struct inode *ip;
	struct	mount *mp;

	ip = getinode(uap->fd);
	if (! ip)
		return;
	mp = (struct mount *)((int)ip->i_fs - offsetof(struct mount, m_filsys));
	u.u_error = statfs1 (mp, uap->buf);
}

void
getfsstat()
{
	register struct a {
		struct	statfs	*buf;
		int	bufsize;
		u_int	flags;
	} *uap = (struct a *)u.u_arg;
	register struct	mount *mp;
	caddr_t	sfsp;
	int	count, maxcount, error;

	maxcount = uap->bufsize / sizeof (struct statfs);
	sfsp = (caddr_t)uap->buf;
	count = 0;
	for (mp = mount; mp < &mount[NMOUNT]; mp++) {
		if (mp->m_inodp == NULL)
			continue;
		if (count < maxcount) {
			error = statfs1 (mp, sfsp);
			if (error) {
				u.u_error = error;
				return;
			}
			sfsp += sizeof (struct statfs);
		}
		count++;
	}
	if (sfsp && count > maxcount)
		u.u_rval = maxcount;
	else
		u.u_rval = count;
}

/*
 * This is somewhat inefficient in that the inode table is scanned for each
 * filesystem but it didn't seem worth a page or two of code on something
 * which only happens every 30 seconds.
 */
static void
syncinodes(fs)
	struct	fs *fs;
{
	register struct inode *ip;

	/*
	 * Write back each (modified) inode.
	 */
	for (ip = inode; ip < inode+NINODE; ip++) {
		/*
		 * Attempt to reduce the overhead by short circuiting the scan if the
		 * inode is not for the filesystem being processed.
		 */
		if (ip->i_fs != fs)
			continue;
		if ((ip->i_flag & ILOCKED) != 0 || ip->i_count == 0 ||
			   (ip->i_flag & (IMOD|IACC|IUPD|ICHG)) == 0)
			continue;
		ip->i_flag |= ILOCKED;
		ip->i_count++;
		iupdat(ip, &time, &time, 0);
		iput(ip);
	}
}

/*
 * 'ufs_sync' is the routine which syncs a single filesystem.  This was
 * created to replace 'update' which 'unmount' called.  It seemed silly to
 * sync _every_ filesystem when unmounting just one filesystem.
 */
int
ufs_sync(mp)
	register struct mount *mp;
{
	register struct fs *fs;
	struct	buf *bp;
	int	error = 0;

	fs = &mp->m_filsys;
	if (fs->fs_fmod && (mp->m_flags & MNT_RDONLY)) {
		printf("fs = %s\n", fs->fs_fsmnt);
		panic("sync: rofs");
	}
	syncinodes(fs);		/* sync the inodes for this filesystem */
	bflush(mp->m_dev);	/* flush dirty data blocks */
	/*
	 * And lastly the superblock, if the filesystem was modified.
	 * Write back modified superblocks. Consistency check that the superblock
	 * of each file system is still in the buffer cache.
	 */
	if (fs->fs_fmod) {
		bp = getblk(mp->m_dev, SUPERB);
		fs->fs_fmod = 0;
		fs->fs_time = time.tv_sec;
		bcopy(fs, bp->b_addr, sizeof (struct fs));
		bwrite(bp);
		error = geterror(bp);
	}
	return(error);
}

/*
 * mode mask for creation of files
 */
void
umask()
{
	register struct a {
		int	mask;
	} *uap = (struct a *)u.u_arg;

	u.u_rval = u.u_cmask;
	u.u_cmask = uap->mask & 07777;
}

/*
 * Seek system call
 */
void
lseek()
{
	register struct file *fp;
	register struct a {
		int	fd;
		off_t	off;
		int	sbase;
	} *uap = (struct a *)u.u_arg;

	if ((fp = getf(uap->fd)) == NULL)
		return;
	if (fp->f_type != DTYPE_INODE) {
		u.u_error = ESPIPE;
		return;
	}
	switch (uap->sbase) {

	case L_INCR:
		fp->f_offset += uap->off;
		break;
	case L_XTND:
		fp->f_offset = uap->off + ((struct inode *)fp->f_data)->i_size;
		break;
	case L_SET:
		fp->f_offset = uap->off;
		break;
	default:
		u.u_error = EINVAL;
		return;
	}
	u.u_rval = fp->f_offset;
}

/*
 * Synch an open file.
 */
void
fsync()
{
	register struct a {
		int	fd;
	} *uap = (struct a *)u.u_arg;
	register struct inode *ip;

	if ((ip = getinode(uap->fd)) == NULL)
		return;
	ilock(ip);
	syncip(ip);
	iunlock(ip);
}

void
utimes()
{
	register struct a {
		char	*fname;
		struct	timeval *tptr;
	} *uap = (struct a *)u.u_arg;
	register struct inode *ip;
	struct	nameidata nd;
	register struct nameidata *ndp = &nd;
	struct timeval tv[2];
	struct vattr vattr;

	VATTR_NULL(&vattr);
	if (uap->tptr == NULL) {
		tv[0].tv_sec = tv[1].tv_sec = time.tv_sec;
		vattr.va_vaflags |= VA_UTIMES_NULL;
	} else {
		u.u_error = copyin ((caddr_t)uap->tptr,(caddr_t)tv,sizeof(tv));
		if (u.u_error)
			return;
	}
	NDINIT (ndp, LOOKUP, FOLLOW, uap->fname);
	if ((ip = namei(ndp)) == NULL)
		return;
	vattr.va_atime = tv[0].tv_sec;
	vattr.va_mtime = tv[1].tv_sec;
	u.u_error = ufs_setattr(ip, &vattr);
	iput(ip);
}
