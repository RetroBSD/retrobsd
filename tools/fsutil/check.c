/*
 * Check 2.xBSD filesystem.
 *
 * Copyright (C) 2006-2011 Serge Vakulenko, <serge@vak.ru>
 *
 * This file is part of RetroBSD project, which is distributed
 * under the terms of the GNU General Public License (GPL).
 * See the accompanying file "COPYING" for more details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "bsdfs.h"

extern int verbose;

#define	MAXDUP		10	/* limit on dup blks (per inode) */
#define	MAXBAD		10	/* limit on bad blks (per inode) */
#define	DUP_LIST_SIZE	100	/* num of dup blocks to remember */
#define LINK_LIST_SIZE	20	/* num zero link cnts to remember */

#define STATE_BITS	2	/* bits per inode state */
#define STATE_MASK	3	/* mask for inode state */
#define STATES_PER_BYTE	4	/* inode states per byte */
#define USTATE		0	/* inode not allocated */
#define FSTATE		1	/* inode is file */
#define DSTATE		2	/* inode is directory */
#define CLEAR		3	/* inode is to be cleared */

#define DATA		1	/* flags for scan_inode() */
#define ADDR		0

#define ALTERD		010	/* values returned by scan functions */
#define KEEPON		004
#define SKIP		002
#define STOP		001

#define outrange(fs,x)	((x) < (fs)->isize || (x) >= (fs)->fsize)

/* block scan function, called by scan_inode for every file block */
typedef int scanner_t (fs_inode_t *inode, unsigned blk, void *arg);

static unsigned char	buf_data [BSDFS_BSIZE];	/* buffer data for scan_directory */
static unsigned 	buf_bno;		/* buffer block number */
static int		buf_dirty;		/* buffer data modified */

static unsigned 	dup_list [DUP_LIST_SIZE]; /* dup block table */
static unsigned 	*dup_end;		/* next entry in dup table */
static unsigned 	*dup_multi;		/* multiple dups part of table */

static unsigned 	bad_link_list [LINK_LIST_SIZE]; /* inos with zero link cnts */
static unsigned 	*bad_link_end;		/* next entry in table */

static char		*block_map;		/* primary blk allocation map */
static char		*free_map;		/* secondary blk allocation map */
static char		*state_map;		/* inode state table */
static int		*link_count;		/* link count table */

static char		pathname [256];		/* file path name for pass2 */
static char		*pathp;			/* pointer to pathname position */
static char		*thisname;		/* ptr to current pathname component */

static char		*lost_found_name = "lost+found";
static unsigned 	lost_found_inode;	/* lost & found directory */

static int		free_list_corrupted;	/* corrupted free list */
static int		bad_blocks;		/* num of bad blks seen (per inode) */
static int		dup_blocks;		/* num of dup blks seen (per inode) */

static char		*find_inode_name;	/* searching for this name */
static unsigned 	find_inode_result;	/* result of inode search */
static unsigned 	lost_inode;		/* lost file to reconnect */

static unsigned long	scan_filesize;		/* file size, decremented during scan */
static unsigned 	total_files;		/* number of files seen */

static void set_inode_state (unsigned inum, int s)
{
	unsigned int byte, shift;

	byte = inum / STATES_PER_BYTE;
	shift = inum % STATES_PER_BYTE * STATE_BITS;
	state_map [byte] &= ~(STATE_MASK << shift);
	state_map [byte] |= s << shift;
}

static int inode_state (unsigned inum)
{
	unsigned int byte, shift;

	byte = inum / STATES_PER_BYTE;
	shift = inum % STATES_PER_BYTE * STATE_BITS;
	return (state_map [byte] >> shift) & STATE_MASK;
}

static int block_is_busy (unsigned blk)
{
	return block_map [blk >> 3] & (1 << (blk & 7));
}

static void mark_block_busy (unsigned blk)
{
	block_map [blk >> 3] |= 1 << (blk & 7);
}

static void mark_block_free (unsigned blk)
{
	block_map [blk >> 3] &= ~(1 << (blk & 7));
}

static void mark_free_list (unsigned blk)
{
	free_map [blk >> 3] |= 1 << (blk & 7);
}

static int in_free_list (unsigned blk)
{
	return free_map [blk >> 3] & (1 << (blk & 7));
}

static void print_io_error (char *s, unsigned blk)
{
	printf ("\nCAN NOT %s: BLK %d\n", s, blk);
}

static void buf_flush (fs_t *fs)
{
	if (buf_dirty && fs->writable) {
/*printf ("WRITE blk %d\n", buf_bno);*/
		if (! fs_write_block (fs, buf_bno, buf_data))
			print_io_error ("WRITE", buf_bno);
	}
	buf_dirty = 0;
}

static int buf_get (fs_t *fs, unsigned blk)
{
	if (buf_bno == blk)
		return 1;
	buf_flush (fs);
/*printf ("read blk %d\n", blk);*/
	if (! fs_read_block (fs, blk, buf_data)) {
		print_io_error ("READ", blk);
		buf_bno = (unsigned)-1;
		return 0;
	}
	buf_bno = blk;
	return 1;
}

/*
 * Scan recursively the indirect block of the inode,
 * and for every block call the given function.
 */
static int scan_indirect_block (fs_inode_t *inode, unsigned blk,
	int double_indirect, int flg, scanner_t *func, void *arg)
{
	unsigned nb;
	int ret, i;
	unsigned char data [BSDFS_BSIZE];

/*printf ("check %siblock %d: \n", double_indirect > 1 ? "triple " : double_indirect ? "double " : "", blk);*/
	if (flg == ADDR) {
		ret = (*func) (inode, blk, arg);
		if (! (ret & KEEPON))
			return ret;
	}
	if (outrange (inode->fs, blk))		/* protect thyself */
		return SKIP;

	if (! fs_read_block (inode->fs, blk, data)) {
		print_io_error ("READ", blk);
		return SKIP;
	}
	for (i = 0; i < BSDFS_BSIZE; i+=2) {
		nb = data [i+1] << 8 | data [i];
		if (nb) {
			if (double_indirect)
				ret = scan_indirect_block (inode, nb,
					double_indirect - 1, flg, func, arg);
			else
				ret = (*func) (inode, nb, arg);

			if (ret & STOP)
				return ret;
		}
	}
	return KEEPON;
}

/*
 * Scan recursively the block list of the inode,
 * and for every block call the given function.
 * Option flg:
 * - when ADDR - call func for both data and indirect blocks
 * - when DATA - only data blocks are processed
 */
static int scan_inode (fs_inode_t *inode, int flg, scanner_t *func, void *arg)
{
	unsigned *ap;
	int ret;

/*printf ("check inode %d: %#o\n", inode->number, inode->mode);*/
	if (((inode->mode & INODE_MODE_FMT) == INODE_MODE_FBLK) ||
	    ((inode->mode & INODE_MODE_FMT) == INODE_MODE_FCHR))
		return KEEPON;
	scan_filesize = inode->size;

	/* Check direct blocks. */
	for (ap = inode->addr; ap < &inode->addr[NDADDR]; ap++) {
		if (*ap) {
			ret = (*func) (inode, *ap, arg);
			if (ret & STOP)
				return ret;
		}
	}
	if (inode->addr[NADDR-3]) {
		/* Check the indirect block. */
		ret = scan_indirect_block (inode, inode->addr[NADDR-3], 0,
			flg, func, arg);
		if (ret & STOP)
			return (ret);
	}
	if (inode->addr[NADDR-2]) {
		/* Check the double indirect block. */
		ret = scan_indirect_block (inode, inode->addr[NADDR-2], 1,
			flg, func, arg);
		if (ret & STOP)
			return (ret);
	}
	if (inode->addr[NADDR-1]) {
		/* Check the last (triple indirect) block. */
		ret = scan_indirect_block (inode, inode->addr[NADDR-1], 2,
			flg, func, arg);
		if (ret & STOP)
			return (ret);
	}
	return KEEPON;
}

static void print_block_error (char *s, unsigned blk, unsigned inum)
{
	printf ("%u %s I=%u\n", blk, s, inum);
}

/*
 * Called once for every block of every file.
 * Mark blocks as busy on block map.
 * If duplicates are found, put them into dup_list.
 */
static int pass1 (fs_inode_t *inode, unsigned blk, void *arg)
{
	unsigned *dlp;
	unsigned *blocks = arg;

/*printf ("pass1 inode %d block %d: \n", inode->number, blk);*/
	if (outrange (inode->fs, blk)) {
		print_block_error ("BAD", blk, inode->number);
		set_inode_state (inode->number, CLEAR);	/* mark for possible clearing */
		if (++bad_blocks >= MAXBAD) {
			printf ("EXCESSIVE BAD BLKS I=%u\n", inode->number);
			return STOP;
		}
		return SKIP;
	}
	if (block_is_busy (blk)) {
		print_block_error ("DUP", blk, inode->number);
		set_inode_state (inode->number, CLEAR);	/* mark for possible clearing */
		if (++dup_blocks >= MAXDUP) {
			printf ("EXCESSIVE DUP BLKS I=%u\n", inode->number);
			return STOP;
		}
		if (dup_end >= &dup_list[DUP_LIST_SIZE]) {
			printf ("DUP TABLE OVERFLOW.\n");
			return STOP;
		}
		for (dlp = dup_list; dlp < dup_multi; dlp++) {
			if (*dlp == blk) {
				*dup_end++ = blk;
				break;
			}
		}
		if (dlp >= dup_multi) {
			*dup_end++ = *dup_multi;
			*dup_multi++ = blk;
		}
	} else {
		if (blocks)
			++*blocks;
		mark_block_busy (blk);
	}
	return KEEPON;
}

static int pass1b (fs_inode_t *inode, unsigned blk, void *arg)
{
	unsigned *dlp;

	if (outrange (inode->fs, blk))
		return SKIP;
	for (dlp = dup_list; dlp < dup_multi; dlp++) {
		if (*dlp == blk) {
			print_block_error ("DUP", blk, inode->number);
			set_inode_state (inode->number, CLEAR);	/* mark for possible clearing */
			*dlp = *--dup_multi;
			*dup_multi = blk;
			return (dup_multi == dup_list ? STOP : KEEPON);
		}
	}
	return KEEPON;
}

/*
 * Read directory, and for every entry call given function.
 * If function altered the contents of entry, then write it back.
 */
static int scan_directory (fs_inode_t *inode, unsigned blk, void *arg)
{
	fs_dirent_t direntry;
	unsigned char *dirp;
	int (*func) () = arg;
	int n;

/*printf ("scan_directory: I=%d, blk=%d\n", inode->number, blk);*/
	if (outrange (inode->fs, blk)) {
		scan_filesize -= BSDFS_BSIZE;
		return SKIP;
	}
	dirp = buf_data;
	while (dirp < &buf_data[BSDFS_BSIZE] && scan_filesize > 0) {
		if (! buf_get (inode->fs, blk)) {
			scan_filesize -= (&buf_data[BSDFS_BSIZE] - dirp);
			return SKIP;
		}
		fs_dirent_unpack (&direntry, dirp);
		if (direntry.reclen == 0)
			break;

		/* For every directory entry, call handler. */
		n = (*func) (inode->fs, &direntry);

		if (n & ALTERD) {
			if (buf_get (inode->fs, blk)) {
				fs_dirent_pack (dirp, &direntry);
				buf_dirty = 1;
			} else
				n &= ~ALTERD;
		}
		if (n & STOP)
			return n;
		dirp += direntry.reclen;
		scan_filesize -= direntry.reclen;
	}
	return (scan_filesize > 0) ? KEEPON : STOP;
}

static void print_inode (fs_inode_t *inode)
{
	char *p;

	printf (" I=%u ", inode->number);
	printf (" OWNER=%d ", inode->uid);
	printf ("MODE=%o\n", inode->mode);
	printf ("SIZE=%ld ", inode->size);
	p = ctime (&inode->mtime);
	printf ("MTIME=%12.12s %4.4s\n", p+4, p+20);
}

static void print_dir_error (fs_t *fs, unsigned inum, char *s)
{
	fs_inode_t inode;

	if (! fs_inode_get (fs, &inode, inum)) {
		printf ("%s  I=%u\nNAME=%s\n", s, inum, pathname);
		return;
	}
	printf ("%s ", s);
	print_inode (&inode);
	printf ("%s=%s\n", ((inode.mode & INODE_MODE_FMT) ==
		INODE_MODE_FDIR) ? "DIR" : "FILE", pathname);
}

static void scan_pass2 (fs_t *fs, unsigned inum);

/*
 * Clear directory entries which refer to duplicated or unallocated inodes.
 * Decrement link counters.
 */
static int pass2 (fs_t *fs, fs_dirent_t *dirp)
{
	int inum, ret = KEEPON;
	fs_inode_t inode;

	inum = dirp->ino;
	if (inum == 0)
		return KEEPON;

	/* Copy file name from dirp to pathp */
	thisname = pathp;
	strcpy (pathp, dirp->name);
	pathp += strlen (pathp);
/*printf ("%s  %d\n", pathname, inum);*/
	if (inum > (fs->isize - 1) * BSDFS_INODES_PER_BLOCK ||
	    inum < BSDFS_ROOT_INODE)
		print_dir_error (fs, inum, "I OUT OF RANGE");
	else {
again:		switch (inode_state (inum)) {
		case USTATE:
			print_dir_error (fs, inum, "UNALLOCATED");
			if (fs->writable) {
				dirp->ino = 0;
				ret |= ALTERD;
				break;
			}
			break;
		case CLEAR:
			print_dir_error (fs, inum, "DUP/BAD");
			if (fs->writable) {
				dirp->ino = 0;
				ret |= ALTERD;
				break;
			}
			if (! fs_inode_get (fs, &inode, inum))
				break;
			set_inode_state (inum, ((inode.mode & INODE_MODE_FMT) ==
				INODE_MODE_FDIR) ? DSTATE : FSTATE);
			goto again;
		case FSTATE:
			--link_count [inum];
			break;
		case DSTATE:
			--link_count [inum];
			scan_pass2 (fs, inum);
		}
	}
	pathp = thisname;
	return ret;
}

/*
 * Traverse directory tree. Call pass2 for every directory entry.
 * Keep current file name in 'pathname'.
 */
static void scan_pass2 (fs_t *fs, unsigned inum)
{
	fs_inode_t inode;
	char *savname;
	unsigned long savsize;

	set_inode_state (inum, FSTATE);
	if (! fs_inode_get (fs, &inode, inum))
		return;
	*pathp++ = '/';
	savname = thisname;
	savsize = scan_filesize;
	scan_inode (&inode, DATA, scan_directory, pass2);
	scan_filesize = savsize;
	thisname = savname;
	*--pathp = 0;
}

/*
 * Find inode number by name.
 * The name is in 'find_inode_name'.
 * Put resulting inode number into 'find_inode_result'.
 */
static int find_inode (fs_t *fs, fs_dirent_t *dirp)
{
	if (dirp->ino == 0)
		return KEEPON;
	if (strcmp (find_inode_name, dirp->name) == 0) {
		if (dirp->ino >= BSDFS_ROOT_INODE &&
		    dirp->ino < (fs->isize-1) * BSDFS_INODES_PER_BLOCK)
			find_inode_result = dirp->ino;
		return STOP;
	}
	return KEEPON;
}

/*
 * Find a free slot and make link to 'lost_inode'.
 * Create filename of a kind "#01234".
 */
static int make_lost_entry (fs_t *fs, fs_dirent_t *dirp)
{
	if (dirp->ino)
		return KEEPON;
	dirp->ino = lost_inode;
	sprintf (dirp->name, "#%05d", dirp->ino);
	return ALTERD | STOP;
}

/*
 * For entry ".." set inode number to 'lost_found_inode'.
 */
static int dotdot_to_lost_found (fs_t *fs, fs_dirent_t *dirp)
{
	if (dirp->name[0] == '.' && dirp->name[1] == '.' &&
	    dirp->name[2] == 0) {
		dirp->ino = lost_found_inode;
		return ALTERD | STOP;
	}
	return KEEPON;
}

/*
 * Return lost+found inode number.
 * TODO: create /lost+found when not available.
 */
static unsigned find_lost_found (fs_t *fs)
{
	fs_inode_t root;

	/* Find lost_found directory inode number. */
	if (! fs_inode_get (fs, &root, BSDFS_ROOT_INODE))
		return 0;
	find_inode_name = lost_found_name;
	find_inode_result = 0;
	scan_inode (&root, DATA, scan_directory, find_inode);
	return find_inode_result;
}

/*
 * Restore a link to parent directory - "..".
 */
static int move_to_lost_found (fs_inode_t *inode)
{
	fs_inode_t lost_found;

	printf ("UNREF %s ", ((inode->mode & INODE_MODE_FMT) ==
		INODE_MODE_FDIR) ? "DIR" : "FILE");
	print_inode (inode);
	if (! inode->fs->writable)
		return 0;

	/* Get lost+found inode. */
	if (lost_found_inode == 0) {
		/* Find lost_found directory inode number. */
		lost_found_inode = find_lost_found (inode->fs);
		if (! lost_found_inode) {
			printf ("SORRY. NO lost+found DIRECTORY\n\n");
			return 0;
		}
	}
	if (! fs_inode_get (inode->fs, &lost_found, lost_found_inode) ||
	    ((lost_found.mode & INODE_MODE_FMT) != INODE_MODE_FDIR) ||
	    inode_state (lost_found_inode) != FSTATE) {
		printf ("SORRY. NO lost+found DIRECTORY\n\n");
		return 0;
	}
	if (lost_found.size % BSDFS_BSIZE) {
		lost_found.size = (lost_found.size + BSDFS_BSIZE - 1) /
			BSDFS_BSIZE * BSDFS_BSIZE;
		if (! fs_inode_save (&lost_found, 1)) {
			printf ("SORRY. ERROR WRITING lost+found I-NODE\n\n");
			return 0;
		}
	}

	/* Put a file to lost+found. */
	lost_inode = inode->number;
	if ((scan_inode (&lost_found, DATA, scan_directory, make_lost_entry) &
	    ALTERD) == 0) {
		printf ("SORRY. NO SPACE IN lost+found DIRECTORY\n\n");
		return 0;
	}
	--link_count [inode->number];

	if ((inode->mode & INODE_MODE_FMT) == INODE_MODE_FDIR) {
		/* For ".." set inode number to lost_found_inode. */
		scan_inode (inode, DATA, scan_directory, dotdot_to_lost_found);
		if (fs_inode_get (inode->fs, &lost_found, lost_found_inode)) {
			lost_found.nlink++;
			++link_count [lost_found.number];
			if (! fs_inode_save (&lost_found, 1)) {
				printf ("SORRY. ERROR WRITING lost+found I-NODE\n\n");
				return 0;
			}
		}
		printf ("DIR I=%u CONNECTED.\n\n", inode->number);
	}
	return 1;
}

/*
 * Mark the block as free. Remove it from dup list.
 */
static int pass4 (fs_inode_t *inode, unsigned blk, void *arg)
{
	unsigned *dlp;
	unsigned *blocks = arg;

	if (outrange (inode->fs, blk))
		return SKIP;
	if (block_is_busy (blk)) {
		/* Free block. */
		for (dlp = dup_list; dlp < dup_end; dlp++)
			if (*dlp == blk) {
				*dlp = *--dup_end;
				return KEEPON;
			}
		mark_block_free (blk);
		if (blocks)
			--*blocks;
	}
	return KEEPON;
}

/*
 * Clear the inode, mark it's blocks as free.
 */
static void clear_inode (fs_t *fs, unsigned inum, char *msg)
{
	fs_inode_t inode;

	if (! fs_inode_get (fs, &inode, inum))
		return;
	if (msg) {
		printf ("%s %s", msg, ((inode.mode & INODE_MODE_FMT) ==
			INODE_MODE_FDIR) ? "DIR" : "FILE");
		print_inode (&inode);
	}
	if (fs->writable) {
		total_files--;
		scan_inode (&inode, ADDR, pass4, 0);
		fs_inode_clear (&inode);
		fs_inode_save (&inode, 1);
	}
}

/*
 * Fix the link count of the inode.
 * If no links - move it to lost+found.
 */
static void adjust_link_count (fs_t *fs, unsigned inum, unsigned lcnt)
{
	fs_inode_t inode;

	if (! fs_inode_get (fs, &inode, inum))
		return;
	if (inode.nlink == lcnt) {
		/* No links to file - move to lost+found. */
		if (! move_to_lost_found (&inode))
			clear_inode (fs, inum, 0);
	} else {
		printf ("LINK COUNT %s", (lost_found_inode==inum) ? lost_found_name :
			(((inode.mode & INODE_MODE_FMT) == INODE_MODE_FDIR) ?
			"DIR" : "FILE"));
		print_inode (&inode);
		printf ("COUNT %d SHOULD BE %d\n",
			inode.nlink, inode.nlink - lcnt);
		if (fs->writable) {
			inode.nlink -= lcnt;
			fs_inode_save (&inode, 1);
		}
	}
}

/*
 * Called from check_free_list() for every block in free list.
 * Count free blocks, detect duplicates.
 */
static int pass5 (fs_t *fs, unsigned blk, unsigned *free_blocks)
{
	if (outrange (fs, blk)) {
		free_list_corrupted = 1;
		if (++bad_blocks >= MAXBAD) {
			printf ("EXCESSIVE BAD BLKS IN FREE LIST.\n");
			return STOP;
		}
		return SKIP;
	}
	if (in_free_list (blk)) {
		free_list_corrupted = 1;
		if (++dup_blocks >= DUP_LIST_SIZE) {
			printf ("EXCESSIVE DUP BLKS IN FREE LIST.\n");
			return STOP;
		}
	} else {
		++*free_blocks;
		mark_free_list (blk);
	}
	return KEEPON;
}

/*
 * Scan a free block list and return a number of free blocks.
 * If the list is corrupted, set 'free_list_corrupted' flag.
 */
static unsigned check_free_list (fs_t *fs)
{
	unsigned *ap, *base;
	unsigned free_blocks, nfree;
	unsigned data [BSDFS_BSIZE / 4];
	unsigned list [NICFREE];
	int i;

	if (fs->nfree == 0)
		return 0;
	free_blocks = 0;
	nfree = fs->nfree;
	base = fs->free;
	for (;;) {
		if (nfree <= 0 || nfree > NICFREE) {
			printf ("BAD FREEBLK COUNT\n");
			free_list_corrupted = 1;
			break;
		}
		ap = base + nfree;
		while (--ap > base) {
			if (pass5 (fs, *ap, &free_blocks) == STOP)
				return free_blocks;
		}
		if (*ap == 0 || pass5 (fs, *ap, &free_blocks) != KEEPON)
			break;
		if (! fs_read_block (fs, *ap, (unsigned char*) data)) {
			print_io_error ("READ", *ap);
			break;
		}
		nfree = data[0];
		for (i=0; i<NICFREE; ++i)
			list [i] = data[i+1];
		base = list;
	}
	return free_blocks;
}

/*
 * Check a list of free inodes.
 */
static void check_free_inode_list (fs_t *fs)
{
	int i;
	unsigned inum;

	for (i=0; i<fs->ninode; i++) {
		inum = fs->inode[i];
		if (inode_state (inum) != USTATE) {
			printf ("ALLOCATED INODE(S) IN IFREE LIST\n");
			if (fs->writable) {
				fs->ninode = i - 1;
				while (i < NICINOD)
					fs->inode [i++] = 0;
				fs->dirty = 1;
			}
			return;
		}
	}
}

/*
 * Build a free block list from scratch.
 */
static unsigned make_free_list (fs_t *fs)
{
	unsigned free_blocks, n;

	fs->nfree = 0;
	fs->flock = 0;
	fs->fmod = 0;
	fs->ilock = 0;
	fs->ronly = 0;
	fs->dirty = 1;
	free_blocks = 0;

	/* Build a list of free blocks */
	fs_block_free (fs, 0);
	for (n = fs->fsize - 1; n >= fs->isize; n--) {
		if (block_is_busy (n))
			continue;
		++free_blocks;
		if (! fs_block_free (fs, n))
			return 0;
	}
	return free_blocks;
}

/*
 * Check filesystem for errors.
 * When readonly - just check and print errors.
 * If the system is open on read/write - fix errors.
 */
int fs_check (fs_t *fs)
{
	fs_inode_t inode;
	int n;
	unsigned inum;
	unsigned block_map_size;		/* number of free blocks */
	unsigned free_blocks;			/* number of free blocks */
	unsigned used_blocks;			/* number of blocks used */
	unsigned last_allocated_inode;		/* hiwater mark of inodes */

	if (fs->isize >= fs->fsize) {
		printf ("Bad filesystem size: total %d blocks with %d inode blocks\n",
			fs->fsize, fs->isize);
		return 0;
	}
	free_list_corrupted = 0;
	total_files = 0;
	used_blocks = 0;
	dup_multi = dup_end = &dup_list[0];
	bad_link_end = &bad_link_list[0];
	lost_found_inode = 0;
	buf_dirty = 0;
	buf_bno = (unsigned) -1;

	/* Allocate memory. */
	block_map_size = (fs->fsize + 7) / 8;
	block_map = calloc (block_map_size, sizeof (*block_map));
	state_map = calloc (((fs->isize-1) * BSDFS_INODES_PER_BLOCK +
		STATES_PER_BYTE) / STATES_PER_BYTE, sizeof (*state_map));
	link_count = calloc ((fs->isize-1) * BSDFS_INODES_PER_BLOCK + 1,
		sizeof (*link_count));
	if (! block_map || ! state_map || ! link_count) {
		printf ("Cannot allocate memory\n");
fatal:		if (block_map)
			free (block_map);
		if (state_map)
			free (state_map);
		if (link_count)
			free (link_count);
		return 0;
	}

	printf ("** Phase 1 - Check Blocks and Sizes\n");
	last_allocated_inode = 0;
	for (inum = 1; inum <= (fs->isize-1) * BSDFS_INODES_PER_BLOCK; inum++) {
		if (! fs_inode_get (fs, &inode, inum))
			continue;
		if (inode.mode & INODE_MODE_FMT) {
/*printf ("inode %d: %#o\n", inode.number, inode.mode);*/
			last_allocated_inode = inum;
			total_files++;
			link_count[inum] = inode.nlink;
			if (link_count[inum] <= 0) {
				if (bad_link_end < &bad_link_list[LINK_LIST_SIZE])
					*bad_link_end++ = inum;
				else {
					printf ("LINK COUNT TABLE OVERFLOW\n");
				}
			}
			set_inode_state (inum, ((inode.mode & INODE_MODE_FMT) ==
				INODE_MODE_FDIR) ? DSTATE : FSTATE);
			bad_blocks = dup_blocks = 0;
			scan_inode (&inode, ADDR, pass1, &used_blocks);
			n = inode_state (inum);
			if (n == DSTATE || n == FSTATE) {
				if ((inode.mode & INODE_MODE_FMT) == INODE_MODE_FDIR &&
				    (inode.size % 4) != 0) {
					printf ("DIRECTORY MISALIGNED I=%u\n\n",
						inode.number);
				}
			}
		}
		else if (inode.mode != 0) {
			printf ("PARTIALLY ALLOCATED INODE I=%u\n", inum);
			if (fs->writable)
				fs_inode_clear (&inode);
		}
		fs_inode_save (&inode, 0);
	}
	if (dup_end != &dup_list[0]) {
		printf ("** Phase 1b - Rescan For More DUPS\n");
		for (inum = 1; inum <= last_allocated_inode; inum++) {
			if (inode_state (inum) == USTATE)
				continue;
			if (! fs_inode_get (fs, &inode, inum))
				continue;
			if (scan_inode (&inode, ADDR, pass1b, 0) & STOP)
				break;
		}
	}

	printf ("** Phase 2 - Check Pathnames\n");
	thisname = pathp = pathname;
	switch (inode_state (BSDFS_ROOT_INODE)) {
	case USTATE:
		printf ("ROOT INODE UNALLOCATED. TERMINATING.\n");
		goto fatal;
	case FSTATE:
		printf ("ROOT INODE NOT DIRECTORY\n");
		if (! fs->writable)
			goto fatal;
		if (! fs_inode_get (fs, &inode, BSDFS_ROOT_INODE))
			goto fatal;
		inode.mode &= ~INODE_MODE_FMT;
		inode.mode |= INODE_MODE_FDIR;
		fs_inode_save (&inode, 1);
		set_inode_state (BSDFS_ROOT_INODE, DSTATE);
	case DSTATE:
		scan_pass2 (fs, BSDFS_ROOT_INODE);
		break;
	case CLEAR:
		printf ("DUPS/BAD IN ROOT INODE\n");
		set_inode_state (BSDFS_ROOT_INODE, DSTATE);
		scan_pass2 (fs, BSDFS_ROOT_INODE);
	}

	printf ("** Phase 3 - Check Connectivity\n");
	for (inum = BSDFS_ROOT_INODE; inum <= last_allocated_inode; inum++) {
		if (inode_state (inum) == DSTATE) {
			unsigned ino;

			find_inode_name = "..";
			ino = inum;
			do {
				if (! fs_inode_get (fs, &inode, ino))
					break;
				find_inode_result = 0;
				scan_inode (&inode, DATA, scan_directory, find_inode);
				if (find_inode_result == 0) {
					/* Parent link lost. */
					if (move_to_lost_found (&inode)) {
						thisname = pathp = pathname;
						*pathp++ = '?';
						scan_pass2 (fs, ino);
					}
					break;
				}
				ino = find_inode_result;
			} while (inode_state (ino) == DSTATE);
		}
	}

	printf ("** Phase 4 - Check Reference Counts\n");
	for (inum = BSDFS_ROOT_INODE; inum <= last_allocated_inode; inum++) {
		switch (inode_state (inum)) {
		case FSTATE:
			n = link_count [inum];
			if (n)
				adjust_link_count (fs, inum, n);
			else {
				unsigned *blp;

				for (blp = bad_link_list; blp < bad_link_end; blp++)
					if (*blp == inum) {
						clear_inode (fs, inum, "UNREF");
						break;
					}
			}
			break;
		case DSTATE:
			clear_inode (fs, inum, "UNREF");
			break;
		case CLEAR:
			clear_inode (fs, inum, "BAD/DUP");
		}
	}
	buf_flush (fs);

	printf ("** Phase 5 - Check Free List\n");
	free (link_count);
	check_free_inode_list (fs);
	free (state_map);
	bad_blocks = dup_blocks = 0;
	free_map = calloc (block_map_size, sizeof (*free_map));
	if (! free_map) {
		printf ("NO MEMORY TO CHECK FREE LIST\n");
		free_list_corrupted = 1;
		free_blocks = 0;
	} else {
		memcpy (free_map, block_map, block_map_size);
		free_blocks = check_free_list (fs);
		free (free_map);
	}
	if (bad_blocks)
		printf ("%d BAD BLKS IN FREE LIST\n", bad_blocks);
	if (dup_blocks)
		printf ("%d DUP BLKS IN FREE LIST\n", dup_blocks);
	if (free_list_corrupted == 0) {
		if (used_blocks + free_blocks != fs->fsize - fs->isize) {
			printf ("%d BLK(S) MISSING\n", fs->fsize -
				fs->isize - used_blocks - free_blocks);
			free_list_corrupted = 1;
		}
	}
	if (free_list_corrupted) {
		printf ("BAD FREE LIST\n");
		if (! fs->writable)
			free_list_corrupted = 0;
	}

	if (free_list_corrupted) {
		printf ("** Phase 6 - Salvage Free List\n");
		free_blocks = make_free_list (fs);
	}

	printf ("%d files %d blocks %d free\n",
		total_files, used_blocks, free_blocks);
	if (fs->modified) {
		time (&fs->utime);
		fs->dirty = 1;
	}
	buf_flush (fs);
	fs_sync (fs, 0);
	if (fs->modified)
		printf ("\n***** FILE SYSTEM WAS MODIFIED *****\n");

	free (block_map);
	return 1;
}
