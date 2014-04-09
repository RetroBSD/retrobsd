/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * The header for buffers in the buffer pool and otherwise used
 * to describe a block i/o request is given here.
 *
 * Each buffer in the pool is usually doubly linked into 2 lists:
 * hashed into a chain by <dev,blkno> so it can be located in the cache,
 * and (usually) on (one of several) queues.  These lists are circular and
 * doubly linked for easy removal.
 *
 * There are currently two queues for buffers:
 * 	one for buffers containing ``useful'' information (the cache)
 *	one for buffers containing ``non-useful'' information
 *		(and empty buffers, pushed onto the front)
 * These queues contain the buffers which are available for
 * reallocation, are kept in lru order.  When not on one of these queues,
 * the buffers are ``checked out'' to drivers which use the available list
 * pointers to keep track of them in their i/o active queues.
 */

/*
 * Bufhd structures used at the head of the hashed buffer queues.
 * We only need three words for these, so this abbreviated
 * definition saves some space.
 */
struct bufhd
{
	int	b_flags;		/* see defines below */
	struct	buf *b_forw, *b_back;	/* fwd/bkwd pointer in chain */
};

struct buf
{
	int	b_flags;		/* see defines below */
	struct	buf *b_forw, *b_back;	/* hash chain (2 way street) */
	struct	buf *av_forw, *av_back;	/* position on free list if not BUSY */
#define	b_actf	av_forw			/* alternate names for driver queue */
#define	b_actl	av_back			/*    head - isn't history wonderful */
	u_int	b_bcount;		/* transfer count */
#define	b_active b_bcount		/* driver queue head: drive active */
	int	b_error;		/* returned after I/O */
	dev_t	b_dev;			/* major+minor device name */
	caddr_t b_addr;			/* core address */
	daddr_t	b_blkno;		/* block # on device */
	u_int	b_resid;		/* words not transferred after error */
#define	b_cylin b_resid			/* disksort */
#define	b_errcnt b_resid		/* while i/o in progress: # retries */
};

/*
 * We never use BQ_LOCKED or BQ_EMPTY, but if you want the 4.X block I/O
 * code to drop in, you have to have BQ_AGE and BQ_LRU *after* the first
 * queue, and it only costs 6 bytes of data space.
 */
#define	BQUEUES		3		/* number of free buffer queues */

#define	BQ_LOCKED	0		/* super-blocks &c */
#define	BQ_LRU		1		/* lru, useful buffers */
#define	BQ_AGE		2		/* rubbish */
#define	BQ_EMPTY	3		/* buffer headers with no memory */

/* Flags to low-level allocation routines. */
#define B_CLRBUF	0x01	/* Request allocated buffer be cleared. */
#define B_SYNC		0x02	/* Do all allocations synchronously. */

#define	bawrite(bp)	{ (bp)->b_flags |= B_ASYNC; bwrite(bp); }
#define	bfree(bp)	(bp)->b_bcount = 0

#ifdef KERNEL
struct inode;

#define	BUFHSZ		16	/* must be power of 2 */
#define	BUFHASH(dev,bn)	((struct buf*) &bufhash [((dev) + bn) & (BUFHSZ - 1)])

extern struct	buf buf[];		/* the buffer pool itself */
extern char	bufdata[];		/* core data */
extern struct	bufhd bufhash[];	/* heads of hash lists */
extern struct	buf bfreelist[];	/* heads of available lists */

/*
 * Assign a buffer for the given block.  If the appropriate
 */
struct buf *getblk (dev_t dev, daddr_t blkno);

/*
 * Allocate a block in the file system.
 */
struct buf *balloc (struct inode *ip, int flags);

/*
 * Get an empty block.
 */
struct buf *geteblk (void);

/*
 * Read in (if necessary) the block and return a buffer pointer.
 */
struct buf *bread (dev_t dev, daddr_t blkno);

/*
 * Read in the block, like bread, but also start I/O on the
 * read-ahead block.
 */
struct buf *breada (dev_t dev, daddr_t blkno, daddr_t rablkno);

/*
 * Write the buffer, waiting for completion. Then release the buffer.
 */
void bwrite (struct buf *bp);

/*
 * Release the buffer, with delayed write.
 */
void bdwrite (struct buf *bp);

/*
 * Mark I/O complete on a buffer.
 */
void biodone (struct buf *bp);

/*
 * Release the buffer, with no I/O implied.
 */
void brelse (struct buf *bp);

/*
 * Wait for I/O completion on the buffer.
 */
void biowait (struct buf *bp);

/*
 * See if the block is associated with some buffer.
 */
int incore (dev_t dev, daddr_t blkno);

/*
 * Make sure all write-behind blocks on dev are flushed out.
 */
void bflush (dev_t dev);

/*
 * Insure that no part of a specified block is in an incore buffer.
 */
void blkflush (dev_t dev, daddr_t blkno);

/*
 * Invalidate in core blocks belonging to closed or umounted filesystem.
 */
void binval (dev_t dev);

/*
 * Pick up the device's error number and pass it to the user.
 */
int geterror (struct buf *bp);

#endif /* KERNEL */

/*
 * These flags are kept in b_flags.
 */
#define	B_WRITE		0x00000		/* non-read pseudo-flag */
#define	B_READ		0x00001		/* read when I/O occurs */
#define	B_DONE		0x00002		/* transaction finished */
#define	B_ERROR		0x00004		/* transaction aborted */
#define	B_BUSY		0x00008		/* not on av_forw/back list */
#define	B_PHYS		0x00010		/* physical IO */
#define	B_MAP		0x00020		/* alloc UNIBUS */
#define	B_WANTED 	0x00040		/* issue wakeup when BUSY goes off */
#define	B_AGE		0x00080		/* delayed write for correct aging */
#define	B_ASYNC		0x00100		/* don't wait for I/O completion */
#define	B_DELWRI 	0x00200		/* write at exit of avail list */
#define	B_TAPE 		0x00400		/* this is a magtape (no bdwrite) */
#define	B_INVAL		0x00800		/* does not contain valid info */
#define	B_BAD		0x01000		/* bad block revectoring in progress */
#define	B_LOCKED	0x02000		/* locked in core (not reusable) */
#define	B_UBAREMAP	0x04000		/* addr UNIBUS virtual, not physical */
#define	B_RAMREMAP	0x08000		/* remapped into ramdisk */

/*
 * Insq/Remq for the buffer hash lists.
 */
#define	bremhash(bp) { \
	(bp)->b_back->b_forw = (bp)->b_forw; \
	(bp)->b_forw->b_back = (bp)->b_back; \
}
#define	binshash(bp, dp) { \
	(bp)->b_forw = (dp)->b_forw; \
	(bp)->b_back = (dp); \
	(dp)->b_forw->b_back = (bp); \
	(dp)->b_forw = (bp); \
}

/*
 * Insq/Remq for the buffer free lists.
 */
#define	bremfree(bp) { \
	(bp)->av_back->av_forw = (bp)->av_forw; \
	(bp)->av_forw->av_back = (bp)->av_back; \
}
#define	binsheadfree(bp, dp) { \
	(dp)->av_forw->av_back = (bp); \
	(bp)->av_forw = (dp)->av_forw; \
	(dp)->av_forw = (bp); \
	(bp)->av_back = (dp); \
}
#define	binstailfree(bp, dp) { \
	(dp)->av_back->av_forw = (bp); \
	(bp)->av_back = (dp)->av_back; \
	(dp)->av_back = (bp); \
	(bp)->av_forw = (dp); \
}

/*
 * Take a buffer off the free list it's on and
 * mark it as being use (B_BUSY) by a device.
 */
#define	notavail(bp) { \
	register int x = splbio(); \
	bremfree(bp); \
	(bp)->b_flags |= B_BUSY; \
	splx(x); \
}
