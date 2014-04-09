/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * file system statistics
 */
#include <sys/fs.h>

#define MNAMELEN 90	/* length of buffer for returned name */

struct statfs {
	short	f_type;			/* type of filesystem (see below) */
	u_short	f_flags;		/* copy of mount flags */
	short	f_bsize;		/* fundamental file system block size */
	short	f_iosize;		/* optimal transfer block size */
	daddr_t	f_blocks;		/* total data blocks in file system */
	daddr_t	f_bfree;		/* free blocks in fs */
	daddr_t	f_bavail;		/* free blocks avail to non-superuser */
	ino_t	f_files;		/* total file nodes in file system */
	ino_t	f_ffree;		/* free file nodes in fs */
	long	f_fsid[2];		/* file system id */
	long	f_spare[5];		/* spare for later */
	char	f_mntonname[MNAMELEN];	/* directory on which mounted */
	char	f_mntfromname[MNAMELEN];/* mounted filesystem */
};

/*
 * File system types.  Since only UFS is supported the others are not
 * specified at this time.
 */
#define	MOUNT_NONE	0
#define	MOUNT_UFS	1	/* Fast Filesystem */
#define	MOUNT_MAXTYPE	1

#define	INITMOUNTNAMES { \
	"none",		/* 0 MOUNT_NONE */ \
	"ufs",		/* 1 MOUNT_UFS */ \
	0,				  \
}

/*
 * Mount structure.
 * One allocated on every mount.
 * Used to find the super block.
 */
struct	mount
{
	dev_t	m_dev;			/* device mounted */
	struct	fs m_filsys;		/* superblock data */
#define	m_flags	m_filsys.fs_flags
	struct	inode *m_inodp;		/* pointer to mounted on inode */
	struct	inode *m_qinod;		/* QUOTA: pointer to quota file */
	char	m_mntfrom [MNAMELEN];	/* /dev/xxxx mounted from */
	char	m_mnton [MNAMELEN];	/* directory mounted on - this is the
					 * full(er) version of fs_fsmnt. */
};

/*
 * Mount flags.
 */
#define	MNT_RDONLY	0x0001		/* read only filesystem */
#define	MNT_SYNCHRONOUS	0x0002		/* file system written synchronously */
#define	MNT_NOEXEC	0x0004		/* can't exec from filesystem */
#define	MNT_NOSUID	0x0008		/* don't honor setuid bits on fs */
#define	MNT_NODEV	0x0010		/* don't interpret special files */
#define	MNT_QUOTA	0x0020		/* quotas are enabled on filesystem */
#define	MNT_ASYNC	0x0040		/* file system written asynchronously */
#define	MNT_NOATIME	0x0080		/* don't update access times */

/*
 * Mask of flags that are visible to statfs().
*/
#define	MNT_VISFLAGMASK	0x0fff

/*
 * filesystem control flags.  The high 4 bits are used for this.  Since NFS
 * support will never be a problem we can avoid making the flags into a 'long.
*/
#define	MNT_UPDATE	0x1000		/* not a real mount, just an update */

/*
 * Flags for various system call interfaces.
 *
 * These aren't used for anything in the system and are present only
 * for source code compatibility reasons.
*/
#define	MNT_WAIT	1
#define	MNT_NOWAIT	2

#ifdef KERNEL

struct	mount mount[NMOUNT];

#else

int getfsstat (struct statfs *buf, int bufsize, unsigned flags);

#endif
