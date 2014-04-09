/*
 * Data structures for 2.xBSD filesystem.
 *
 * Copyright (C) 2006-2011 Serge Vakulenko, <serge@vak.ru>
 *
 * This file is part of RetroBSD project, which is distributed
 * under the terms of the GNU General Public License (GPL).
 * See the accompanying file "COPYING" for more details.
 */
#define BSDFS_BSIZE		1024	/* block size */
#define BSDFS_ROOT_INODE	2	/* root directory in inode 2 */
#define BSDFS_LOSTFOUND_INODE	3	/* lost+found directory in inode 3 */
#define BSDFS_SWAP_INODE	4	/* swap file in inode 4 */
#define BSDFS_INODES_PER_BLOCK	16	/* inodes per block */

#define	NICINOD		32		/* number of superblock inodes */
#define	NICFREE		200		/* number of superblock free blocks */

/*
 * 28 of the di_addr address bytes are used; 7 addresses of 4
 * bytes each: 4 direct (4Kb directly accessible) and 3 indirect.
 */
#define	NDADDR	4			/* direct addresses in inode */
#define	NIADDR	3			/* indirect addresses in inode */
#define	NADDR	(NDADDR + NIADDR)	/* total addresses in inode */

/*
 * NINDIR is the number of indirects in a file system block.
 */
#define	NINDIR		(DEV_BSIZE / sizeof(daddr_t))
#define	NSHIFT		8		/* log2(NINDIR) */
#define	NMASK		0377L		/* NINDIR - 1 */

/*
 * The path name on which the file system is mounted is maintained
 * in fs_fsmnt. MAXMNTLEN defines the amount of space allocated in
 * the super block for this name.
 */
#define MAXMNTLEN	28

#define MAXNAMLEN	63

#define	FSMAGIC1	('F' | 'S'<<8 | '<'<<16 | '<'<<24)
#define	FSMAGIC2	('>' | '>'<<8 | 'F'<<16 | 'S'<<24)

typedef struct {
	const char	*filename;
	int		fd;
	unsigned long	seek;
	int		writable;
	int		dirty;		/* sync needed */
	int		modified;	/* write_block was called */

	unsigned 	isize;		/* size in blocks of superblock + I list */
	unsigned 	fsize;		/* size in blocks of entire volume */
	unsigned 	swapsz;		/* size in blocks of swap area */
	unsigned 	nfree;		/* number of in core free blocks (0-100) */
	unsigned 	free [NICFREE];	/* in core free blocks */
	unsigned 	ninode;		/* number of in core I nodes (0-100) */
	unsigned 	inode [NICINOD]; /* in core free I nodes */
	unsigned 	flock;		/* lock during free list manipulation */
	unsigned 	ilock;		/* lock during I list manipulation */
	unsigned 	fmod;		/* super block modified flag */
	unsigned 	ronly;		/* mounted read-only flag */
	long		utime;		/* current date of last update */
	unsigned	tfree;		/* total free blocks */
	unsigned	tinode;		/* total free inodes */
	char		fsmnt [MAXMNTLEN]; /* ordinary file mounted on */
	unsigned	lasti;		/* start place for circular search */
	unsigned	nbehind;	/* est # free inodes before s_lasti */
	unsigned	flags;		/* mount time flags */
} fs_t;

typedef struct {
	fs_t		*fs;
	unsigned 	number;
	int		dirty;		/* save needed */

	unsigned short	mode;		/* file type and access mode */
#define	INODE_MODE_FMT		0170000	/* type of file */
#define	INODE_MODE_FCHR		 020000	/* character special */
#define	INODE_MODE_FDIR		 040000	/* directory */
#define	INODE_MODE_FBLK		 060000	/* block special */
#define	INODE_MODE_FREG		0100000	/* regular */
#define	INODE_MODE_FLNK		0120000	/* symbolic link */
#define	INODE_MODE_FSOCK	0140000	/* socket */
#define	INODE_MODE_SUID		  04000	/* set user id on execution */
#define	INODE_MODE_SGID		  02000	/* set group id on execution */
#define	INODE_MODE_SVTX		  01000	/* save swapped text even after use */
#define	INODE_MODE_READ		   0400	/* read, write, execute permissions */
#define	INODE_MODE_WRITE	   0200
#define	INODE_MODE_EXEC		   0100

	unsigned short	nlink;		/* directory entries */
	unsigned 	uid;		/* owner */
	unsigned 	gid;		/* group */
	unsigned long	size;		/* size */
	unsigned 	addr [7];	/* device addresses constituting file */
	unsigned 	flags;		/* user defined flags */
/*
 * Super-user and owner changeable flags.
 */
#define	USER_SETTABLE	0x00ff		/* mask of owner changeable flags */
#define	USER_NODUMP	0x0001		/* do not dump file */
#define	USER_IMMUTABLE	0x0002		/* file may not be changed */
#define	USER_APPEND	0x0004		/* writes to file may only append */
/*
 * Super-user changeable flags.
 */
#define	SYS_SETTABLE	0xff00		/* mask of superuser changeable flags */
#define	SYS_ARCHIVED	0x0100		/* file is archived */
#define	SYS_IMMUTABLE	0x0200		/* file may not be changed */
#define	SYS_APPEND	0x0400		/* writes to file may only append */

	long		atime;		/* time last accessed */
	long		mtime;		/* time last modified */
	long		ctime;		/* time created */
} fs_inode_t;

typedef struct {
	unsigned 	ino;
	unsigned 	reclen;
	unsigned 	namlen;
	char		name [MAXNAMLEN+1];
} fs_dirent_t;

typedef void (*fs_directory_scanner_t) (fs_inode_t *dir,
	fs_inode_t *file, char *dirname, char *filename, void *arg);

typedef struct {
	fs_inode_t	inode;
	int		writable;	/* write allowed */
	unsigned long	offset;		/* current i/o offset */
} fs_file_t;

int fs_seek (fs_t *fs, unsigned long offset);
int fs_read8 (fs_t *fs, unsigned char *val);
int fs_read16 (fs_t *fs, unsigned short *val);
int fs_read32 (fs_t *fs, unsigned *val);
int fs_write8 (fs_t *fs, unsigned char val);
int fs_write16 (fs_t *fs, unsigned short val);
int fs_write32 (fs_t *fs, unsigned val);

int fs_read (fs_t *fs, unsigned char *data, int bytes);
int fs_write (fs_t *fs, unsigned char *data, int bytes);

int fs_open (fs_t *fs, const char *filename, int writable);
void fs_close (fs_t *fs);
int fs_sync (fs_t *fs, int force);
int fs_create (fs_t *fs, const char *filename, unsigned kbytes,
        unsigned swap_kbytes);
int fs_check (fs_t *fs);
void fs_print (fs_t *fs, FILE *out);

int fs_inode_get (fs_t *fs, fs_inode_t *inode, unsigned inum);
int fs_inode_save (fs_inode_t *inode, int force);
void fs_inode_clear (fs_inode_t *inode);
void fs_inode_truncate (fs_inode_t *inode);
void fs_inode_print (fs_inode_t *inode, FILE *out);
int fs_inode_read (fs_inode_t *inode, unsigned long offset,
	unsigned char *data, unsigned long bytes);
int fs_inode_write (fs_inode_t *inode, unsigned long offset,
	unsigned char *data, unsigned long bytes);
int fs_inode_alloc (fs_t *fs, fs_inode_t *inode);
int fs_inode_by_name (fs_t *fs, fs_inode_t *inode, char *name,
	int op, int mode);
int inode_build_list (fs_t *fs);

int fs_write_block (fs_t *fs, unsigned bnum, unsigned char *data);
int fs_read_block (fs_t *fs, unsigned bnum, unsigned char *data);
int fs_block_free (fs_t *fs, unsigned int bno);
int fs_block_alloc (fs_t *fs, unsigned int *bno);
int fs_indirect_block_free (fs_t *fs, unsigned int bno);
int fs_double_indirect_block_free (fs_t *fs, unsigned int bno);
int fs_triple_indirect_block_free (fs_t *fs, unsigned int bno);

void fs_directory_scan (fs_inode_t *inode, char *dirname,
	fs_directory_scanner_t scanner, void *arg);
void fs_dirent_pack (unsigned char *data, fs_dirent_t *dirent);
void fs_dirent_unpack (fs_dirent_t *dirent, unsigned char *data);

int fs_file_create (fs_t *fs, fs_file_t *file, char *name, int mode);
int fs_file_open (fs_t *fs, fs_file_t *file, char *name, int wflag);
int fs_file_read (fs_file_t *file, unsigned char *data,
	unsigned long bytes);
int fs_file_write (fs_file_t *file, unsigned char *data,
	unsigned long bytes);
int fs_file_close (fs_file_t *file);
