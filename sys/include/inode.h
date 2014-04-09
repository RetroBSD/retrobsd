/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * The I node is the focus of all file activity in UNIX.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, text file, and the root.
 * An inode is 'named' by its dev/inumber pair. (iget/iget.c)
 * Data in icommon1 and icommon2 is read in from permanent inode on volume.
 */

/*
 * 28 of the di_addr address bytes are used; 7 addresses of 4
 * bytes each: 4 direct (4Kb directly accessible) and 3 indirect.
 */
#define	NDADDR	4			/* direct addresses in inode */
#define	NIADDR	3			/* indirect addresses in inode */
#define	NADDR	(NDADDR + NIADDR)	/* total addresses in inode */

struct icommon1 {
	u_short	ic_mode;		/* mode and type of file */
	u_short	ic_nlink;		/* number of links to file */
	uid_t	ic_uid;			/* owner's user id */
	gid_t	ic_gid;			/* owner's group id */
	off_t	ic_size;		/* number of bytes in file */
};

struct icommon2 {
	time_t	ic_atime;		/* time last accessed */
	time_t	ic_mtime;		/* time last modified */
	time_t	ic_ctime;		/* time created */
};

struct inode {
	struct inode	*i_chain[2];	/* must be first */
	u_int		i_flag;
	u_int		i_count;	/* reference count */
	dev_t		i_dev;		/* device where inode resides */
	ino_t		i_number;	/* i number, 1-to-1 with device address */
	u_int		i_id;		/* unique identifier */
	struct fs	*i_fs;		/* file sys associated with this inode */
	union {
		struct {
			u_short	I_shlockc;	/* count of shared locks */
			u_short	I_exlockc;	/* count of exclusive locks */
		} i_l;
		struct	proc *I_rsel;	/* pipe read select */
	} i_un0;
	union {
		struct	proc *I_wsel;	/* pipe write select */
	} i_un1;
	union {
		daddr_t	I_addr[NADDR];		/* normal file/directory */
		struct {
			daddr_t	I_db[NDADDR];	/* normal file/directory */
			daddr_t	I_ib[NIADDR];
		} i_f;
		struct {
			/*
			 * the dummy field is here so that the de/compression
			 * part of the iget/iput routines works for special
			 * files.
			 */
			u_int	I_dummy;
			dev_t	I_rdev;		/* dev type */
		} i_d;
	} i_un2;
	union {
		daddr_t	if_lastr;		/* last read (read-ahead) */
		struct	{
			struct inode  *if_freef; /* free list forward */
			struct inode **if_freeb; /* free list back */
		} i_fr;
	} i_un3;
	struct icommon1 i_ic1;
	u_int	i_flags;			/* user changeable flags */
	struct icommon2 i_ic2;
};

/*
 * Inode structure as it appears on
 * a disk block.
 */
struct dinode {
	struct	icommon1 di_icom1;
	daddr_t	di_addr[NADDR];		/* 7 block addresses 4 bytes each */
	u_int	di_reserved[1];		/* pad of 4 to make total size 64 */
	u_int	di_flags;
	struct	icommon2 di_icom2;
};

#define	i_mode		i_ic1.ic_mode
#define	i_nlink		i_ic1.ic_nlink
#define	i_uid		i_ic1.ic_uid
#define	i_gid		i_ic1.ic_gid
#define	i_size		i_ic1.ic_size
#define	i_shlockc	i_un0.i_l.I_shlockc
#define	i_exlockc	i_un0.i_l.I_exlockc
#define	i_rsel		i_un0.I_rsel
#define	i_wsel		i_un1.I_wsel
#define	i_db		i_un2.i_f.I_db
#define	i_ib		i_un2.i_f.I_ib
#define	i_atime		i_ic2.ic_atime
#define	i_mtime		i_ic2.ic_mtime
#define	i_ctime		i_ic2.ic_ctime
#define	i_rdev		i_un2.i_d.I_rdev
#define	i_addr		i_un2.I_addr
#define	i_dummy		i_un2.i_d.I_dummy
#define	i_lastr		i_un3.if_lastr
#define	i_forw		i_chain[0]
#define	i_back		i_chain[1]
#define	i_freef		i_un3.i_fr.if_freef
#define	i_freeb		i_un3.i_fr.if_freeb

#define di_ic1		di_icom1
#define di_ic2		di_icom2
#define	di_mode		di_ic1.ic_mode
#define	di_nlink	di_ic1.ic_nlink
#define	di_uid		di_ic1.ic_uid
#define	di_gid		di_ic1.ic_gid
#define	di_size		di_ic1.ic_size
#define	di_atime	di_ic2.ic_atime
#define	di_mtime	di_ic2.ic_mtime
#define	di_ctime	di_ic2.ic_ctime

#ifdef KERNEL
struct stat;

/*
 * Invalidate an inode. Used by the namei cache to detect stale
 * information. In order to save space and also reduce somewhat the
 * overhead - the i_id field is made into a u_short.  If a pdp-11 can
 * invalidate 100 inodes per second, the cache will have to be invalidated
 * in about 11 minutes.  Ha!
 * Assumes the cacheinvalall routine will map the namei cache.
 */
void cinvalall (void);

#define cacheinval(ip) \
	(ip)->i_id = ++nextinodeid; \
	if (nextinodeid == 0) \
		cinvalall();

u_int nextinodeid;		/* unique id generator */

extern struct inode inode[];	/* the inode table itself */
struct inode *rootdir;		/* pointer to inode of root directory */

/*
 * Initialize hash links for inodes and build inode free list.
 */
void ihinit (void);

/*
 * Get an inode pointer of a file descriptor.
 */
struct inode *getinode (int fdes);

/*
 * Allocate an inode in the file system.
 */
struct inode *ialloc (struct inode *pip);

/*
 * Look up an inode by device, inumber.
 */
struct inode *iget (dev_t dev, struct fs *fs, ino_t ino);

/*
 * Dereference an inode structure. On the last reference,
 * write the inode out and deallocate the file.
 */
void iput (struct inode *ip);

/*
 * Make a new file.
 */
struct nameidata;
struct inode *maknode (int mode, struct nameidata *ndp);

/*
 * Open inode: initialize and validate special files.
 */
int openi (struct inode *ip, int mode);

/*
 * Close inode: call the device driver for special (IBLK, ICHR) files.
*/
int closei (struct inode *ip, int flag);

/*
 * Convert a pathname into a pointer to a locked inode.
 */
struct inode *namei (struct nameidata *ndp);

enum uio_rw;
int rdwri (enum uio_rw rw, struct inode *ip, caddr_t base, int len,
	off_t offset, int ioflg, int *aresid);

struct uio;
int rwip (struct inode *ip, struct uio *uio, int ioflag);

/*
 * Check mode permission on inode pointer.
 */
int access (struct inode *ip, int mode);

/*
 * Change the mode on a file.
 */
int chmod1 (struct inode *ip, int mode);

/*
 * Change an owner of a file.
 */
int chown1 (struct inode *ip, int uid, int gid);

/*
 * Lock/unlock an inode.
 */
void ilock (struct inode *ip);
void iunlock (struct inode *ip);

/*
 * Get inode statistics.
 */
int ino_stat (struct inode *ip, struct stat *sb);

/*
 * Truncate the inode ip to at most length size.
 */
void itrunc (struct inode *oip, u_long length, int ioflags);

/*
 * Update the inode with the current time.
 */
struct timeval;
void iupdat (struct inode *ip, struct timeval *ta, struct timeval *tm,
	int waitfor);

void irele (struct inode *ip);

/*
 * Free an inode.
 */
void ifree (struct inode *ip, ino_t ino);

/*
 * Free a block or fragment.
 */
void free (struct inode *ip, daddr_t bno);

/*
 * Flush all the blocks associated with an inode.
 */
void syncip (struct inode *ip);

/*
 * Remove any inodes in the inode cache belonging to dev.
 */
int iflush (dev_t dev);

/*
 * Convert a pointer to an inode into a reference to an inode.
 */
void igrab (struct inode *ip);

/*
 * Check if source directory is in the path of the target directory.
 */
int checkpath (struct inode *source, struct inode *target);

/*
 * Check if a directory is empty or not.
 */
int dirempty (struct inode *ip, ino_t parentino);

/*
 * Rewrite an existing directory entry to point at the inode supplied.
 */
void dirrewrite (struct inode *dp, struct inode *ip, struct nameidata *ndp);

/*
 * Check that device is mounted somewhere.
 */
int ufs_mountedon (dev_t dev);

/*
 * Set the attributes on a file.  This was placed here because ufs_syscalls
 * is too large already (it will probably be split into two files eventually).
 */
struct vattr;
int ufs_setattr (struct inode *ip, struct vattr *vap);

/*
 * Cache flush, called when filesys is umounted.
 */
void nchinval (dev_t dev);

#endif /* KERNEL */

/* i_flag */
#define	ILOCKED		0x1		/* inode is locked */
#define	IUPD		0x2		/* file has been modified */
#define	IACC		0x4		/* inode access time to be updated */
#define	IMOUNT		0x8		/* inode is mounted on */
#define	IWANT		0x10		/* some process waiting on lock */
#define	ITEXT		0x20		/* inode is pure text prototype */
#define	ICHG		0x40		/* inode has been changed */
#define	ISHLOCK		0x80		/* file has shared lock */
#define	IEXLOCK		0x100		/* file has exclusive lock */
#define	ILWAIT		0x200		/* someone waiting on file lock */
#define	IMOD		0x400		/* inode has been modified */
#define	IRENAME		0x800		/* inode is being renamed */
#define	IPIPE		0x1000		/* inode is a pipe */
#define	IRCOLL		0x2000		/* read select collision on pipe */
#define	IWCOLL		0x4000		/* write select collision on pipe */
#define	IXMOD		0x8000		/* inode is text, but impure (XXX) */

/* i_mode */
#define	IFMT		0170000		/* type of file */
#define	IFCHR		0020000		/* character special */
#define	IFDIR		0040000		/* directory */
#define	IFBLK		0060000		/* block special */
#define	IFREG		0100000		/* regular */
#define	IFLNK		0120000		/* symbolic link */
#define	IFSOCK		0140000		/* socket */
#define	ISUID		04000		/* set user id on execution */
#define	ISGID		02000		/* set group id on execution */
#define	ISVTX		01000		/* save swapped text even after use */
#define	IREAD		0400		/* read, write, execute permissions */
#define	IWRITE		0200
#define	IEXEC		0100

#ifdef KERNEL
/*
 * Flags for va_cflags.
 */
#define	VA_UTIMES_NULL	0x01		/* utimes argument was NULL */

/*
 * Flags for ioflag.
 */
#define	IO_UNIT		0x01		/* do I/O as atomic unit */
#define	IO_APPEND	0x02		/* append write to end */
#define	IO_SYNC		0x04		/* do I/O synchronously */
/*#define IO_NODELOCKED	0x08		   not implemented */
#define	IO_NDELAY	0x10		/* FNDELAY flag set in file table */


/*
 * This is a bit of a misnomer.  2.11BSD does not have 'vnodes' but it was
 * easier/simpler to keep the name 'vattr' than changing the name to something
 * like 'iattr'.
 *
 * This structure is a _subset_ of 4.4BSD's vnode attribute structure.  ONLY
 * those attributes which can be *changed by the user* are present.  Since we
 * do not have vnodes why initialize (and carry around) un-used members.
 */
struct vattr {
	mode_t	va_mode;
	uid_t	va_uid;
	gid_t	va_gid;
	off_t	va_size;
	time_t	va_atime;
	time_t	va_mtime;
	u_int	va_flags;
	u_int	va_vaflags;
};

/*
 * Token indicating no attribute value yet assigned.
 */
#define	VNOVAL	(-1)

/*
 * Initialize a inode attribute structure.
 */
#define	VATTR_NULL(vp) { \
	(vp)->va_mode = VNOVAL; \
	(vp)->va_uid = VNOVAL; \
	(vp)->va_gid = VNOVAL; \
	(vp)->va_size = VNOVAL; \
	(vp)->va_atime = VNOVAL; \
	(vp)->va_mtime = VNOVAL; \
	(vp)->va_flags = VNOVAL; \
	(vp)->va_vaflags = VNOVAL; }

/*
 * N.B:  If the above structure changes be sure to modify the function
 * vattr_null in pdp/mch_xxx.s!
 */
#endif

#define	ILOCK(ip) { \
	while ((ip)->i_flag & ILOCKED) { \
		(ip)->i_flag |= IWANT; \
		sleep((caddr_t)(ip), PINOD); \
	} \
	(ip)->i_flag |= ILOCKED; \
}

#define	IUNLOCK(ip) { \
	(ip)->i_flag &= ~ILOCKED; \
	if ((ip)->i_flag&IWANT) { \
		(ip)->i_flag &= ~IWANT; \
		wakeup((caddr_t)(ip)); \
	} \
}

#define	IUPDAT(ip, t1, t2, waitfor) { \
	if (ip->i_flag&(IUPD|IACC|ICHG|IMOD)) \
		iupdat(ip, t1, t2, waitfor); \
}

#define	ITIMES(ip, t1, t2) { \
	if ((ip)->i_flag&(IUPD|IACC|ICHG)) { \
		(ip)->i_flag |= IMOD; \
		if ((ip)->i_flag&IACC) \
			(ip)->i_atime = (t1)->tv_sec; \
		if ((ip)->i_flag&IUPD) \
			(ip)->i_mtime = (t2)->tv_sec; \
		if ((ip)->i_flag&ICHG) \
			(ip)->i_ctime = time.tv_sec; \
		(ip)->i_flag &= ~(IACC|IUPD|ICHG); \
	} \
}
