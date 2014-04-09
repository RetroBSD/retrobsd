/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifndef	_SYS_FS_H_
#define	_SYS_FS_H_

/*
 * The root inode is the root of the file system.
 * Inode 0 can't be used for normal purposes and
 * historically bad blocks were linked to inode 1,
 * thus the root inode is 2. (inode 1 is no longer used for
 * this purpose, however numerous dump tapes make this
 * assumption, so we are stuck with it)
 * The lost+found directory is given the next available
 * inode when it is created by ``mkfs''.
 */
#define SBSIZE		DEV_BSIZE
#define	SUPERB		((daddr_t)0)	/* block number of the super block */

#define	ROOTINO		((ino_t)2)	/* i number of all roots */
#define	LOSTFOUNDINO	(ROOTINO + 1)

#define	NICINOD		32		/* number of superblock inodes */
#define	NICFREE		200		/* number of superblock free blocks */

/*
 * The path name on which the file system is mounted is maintained
 * in fs_fsmnt. MAXMNTLEN defines the amount of space allocated in
 * the super block for this name.
 */
#define MAXMNTLEN	28

/*
 * Super block for a file system.  NOTE:  The 'fs_flock' and 'fs_ilock'
 * fields MUST be on an even byte boundary because they are used as sleep()
 * channels and odd values specify a network sleep().
 */
struct	fs
{
	u_int	fs_magic1;		/* magic word */
	u_int	fs_isize;		/* first block after i-list */
	u_int	fs_fsize;		/* size in blocks of entire volume */
	u_int	fs_swapsz;		/* size in blocks of swap area */
	int	fs_nfree;		/* number of addresses in fs_free */
	daddr_t	fs_free [NICFREE];	/* free block list */
	int	fs_ninode;		/* number of inodes in fs_inode */
	ino_t	fs_inode [NICINOD];	/* free inode list */
	int 	fs_flock;		/* lock during free list manipulation */
	int 	fs_fmod;		/* super block modified flag */
	int  	fs_ilock;		/* lock during i-list manipulation */
	int  	fs_ronly;		/* mounted read-only flag */
	time_t	fs_time;		/* last super block update */
	u_int	fs_tfree;		/* total free blocks */
	ino_t	fs_tinode;		/* total free inodes */
	char	fs_fsmnt [MAXMNTLEN];	/* ordinary file mounted on */
	ino_t	fs_lasti;		/* start place for circular search */
	ino_t	fs_nbehind;		/* est # free inodes before s_lasti */
	u_int	fs_flags;		/* mount time flags */
	u_int	fs_magic2;		/* magic word */
/* actually longer */
};

struct	fblk {
	int	df_nfree;		/* number of addresses in df_free */
	daddr_t	df_free [NICFREE];	/* free block list */
};

#define	FSMAGIC1	('F' | 'S'<<8 | '<'<<16 | '<'<<24)
#define	FSMAGIC2	('>' | '>'<<8 | 'F'<<16 | 'S'<<24)

/*
 * Turn file system block numbers into disk block addresses.
 * This maps file system blocks to device size blocks.
 */
#define	fsbtodb(b)	((daddr_t) (b))
#define	dbtofsb(b)	((daddr_t) (b))

/*
 * Macros for handling inode numbers:
 *     inode number to file system block offset.
 *     inode number to file system block address.
 */
#define	itoo(x)		((int)(((x) + INOPB - 1) % INOPB))
#define	itod(x)		((daddr_t)((((u_int)(x) + INOPB - 1) / INOPB)))

/*
 * The following macros optimize certain frequently calculated
 * quantities by using shifts and masks in place of divisions
 * modulos and multiplications.
 */
#define blkoff(loc)		/* calculates (loc % fs->fs_bsize) */ \
	((loc) & DEV_BMASK)
#define lblkno(loc)		/* calculates (loc / fs->fs_bsize) */ \
	((unsigned) (loc) >> DEV_BSHIFT)

/*
 * Determine the number of available blocks given a
 * percentage to hold in reserve
 */
#define freespace(fs, percentreserved) \
	((fs)->fs_tfree - ((fs)->fs_fsize - \
	(fs)->fs_isize) * (percentreserved) / 100)

/*
 * INOPB is the number of inodes in a secondary storage block.
 */
#define INOPB		16		/* MAXBSIZE / sizeof(dinode) */

/*
 * NINDIR is the number of indirects in a file system block.
 */
#define	NINDIR		(DEV_BSIZE / sizeof(daddr_t))
#define	NSHIFT		8		/* log2(NINDIR) */
#define	NMASK		0377L		/* NINDIR - 1 */

/*
 * We continue to implement pipes within the file system because it would
 * be pretty tough for us to handle 10 4K blocked pipes on a 1M machine.
 *
 * 4K is the allowable buffering per write on a pipe.  This is also roughly
 * the max size of the file created to implement the pipe.  If this size is
 * bigger than 4096, pipes will be implemented with large files, which is
 * probably not good.
 */
#define	MAXPIPSIZ	(NDADDR * MAXBSIZE)

#ifdef KERNEL
struct inode;

/*
 * Map a device number into a pointer to the incore super block.
 */
struct fs *getfs (dev_t dev);

/*
 * Mount a filesystem on the given directory inode.
 */
struct fs *mountfs (dev_t dev, int flags, struct inode *ip);

void mount_updname (struct fs *fs, char *on, char *from,
	int lenon, int lenfrom);

/*
 * Sync a single filesystem.
 */
struct mount;
int ufs_sync (struct mount *mp);

/*
 * Check that a specified block number is in range.
 */
int badblock (struct fs *fp, daddr_t bn);

/*
 * Print the name of a file system with an error diagnostic.
 */
void fserr (struct fs *fp, char *message);

#endif /* KERNEL */

#endif /* _SYS_FS_H_ */
