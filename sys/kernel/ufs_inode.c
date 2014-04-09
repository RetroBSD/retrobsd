/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "user.h"
#include "proc.h"
#include "inode.h"
#include "fs.h"
#include "mount.h"
#include "kernel.h"
#include "buf.h"
#include "systm.h"
#include "syslog.h"

#define	INOHSZ	16		/* must be power of two */
#define	INOHASH(dev,ino)	(((dev)+(ino))&(INOHSZ-1))

union ihead {				/* inode LRU cache, stolen */
	union  ihead *ih_head[2];
	struct inode *ih_chain[2];
} ihead[INOHSZ];

struct inode *ifreeh, **ifreet;

/*
 * Initialize hash links for inodes
 * and build inode free list.
 */
void
ihinit()
{
	register int i;
	register struct inode *ip = inode;
	register union  ihead *ih = ihead;

	for (i = INOHSZ; --i >= 0; ih++) {
		ih->ih_head[0] = ih;
		ih->ih_head[1] = ih;
	}
	ifreeh = ip;
	ifreet = &ip->i_freef;
	ip->i_freeb = &ifreeh;
	ip->i_forw = ip;
	ip->i_back = ip;
	for (i = NINODE; --i > 0; ) {
		++ip;
		ip->i_forw = ip;
		ip->i_back = ip;
		*ifreet = ip;
		ip->i_freeb = ifreet;
		ifreet = &ip->i_freef;
	}
	ip->i_freef = NULL;
}

/*
 * Find an inode if it is incore.
 */
struct inode *
ifind(dev, ino)
	register dev_t dev;
	register ino_t ino;
{
	register struct inode *ip;
	union ihead *ih;

	ih = &ihead[INOHASH(dev, ino)];
	for (ip = ih->ih_chain[0]; ip != (struct inode *)ih; ip = ip->i_forw)
		if (ino == ip->i_number && dev == ip->i_dev)
			return(ip);
	return((struct inode *)NULL);
}

/*
 * Look up an inode by device,inumber.
 * If it is in core (in the inode structure),
 * honor the locking protocol.
 * If it is not in core, read it in from the
 * specified device.
 * If the inode is mounted on, perform
 * the indicated indirection.
 * In all cases, a pointer to a locked
 * inode structure is returned.
 *
 * panic: no imt -- if the mounted file
 *	system is not in the mount table.
 *	"cannot happen"
 */
struct inode *
iget(dev, fs, ino)
	dev_t dev;
	register struct fs *fs;
	ino_t ino;
{
	register struct inode *ip;
	union ihead *ih;
	struct buf *bp;
	struct dinode *dp;
loop:
	ih = &ihead[INOHASH(dev, ino)];
	for (ip = ih->ih_chain[0]; ip != (struct inode *)ih; ip = ip->i_forw)
		if (ino == ip->i_number && dev == ip->i_dev) {
			/*
			 * Following is essentially an inline expanded
			 * copy of igrab(), expanded inline for speed,
			 * and so that the test for a mounted on inode
			 * can be deferred until after we are sure that
			 * the inode isn't busy.
			 */
			if ((ip->i_flag&ILOCKED) != 0) {
				ip->i_flag |= IWANT;
				sleep((caddr_t)ip, PINOD);
				goto loop;
			}
			if ((ip->i_flag&IMOUNT) != 0) {
				register struct mount *mp;

				for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
					if(mp->m_inodp == ip) {
						dev = mp->m_dev;
						fs = &mp->m_filsys;
						ino = ROOTINO;
						goto loop;
					}
				panic("no imt");
			}
			if (ip->i_count == 0) {		/* ino on free list */
				register struct inode *iq;

				iq = ip->i_freef;
				if (iq)
					iq->i_freeb = ip->i_freeb;
				else
					ifreet = ip->i_freeb;
				*ip->i_freeb = iq;
				ip->i_freef = NULL;
				ip->i_freeb = NULL;
			}
			ip->i_count++;
			ip->i_flag |= ILOCKED;
			return(ip);
		}

        ip = ifreeh;
	if (ip == NULL) {
                log(LOG_ERR, "inode: table full\n");
		u.u_error = ENFILE;
		return(NULL);
	}
	if (ip->i_count)
		panic("free inode isn't");
	{
	register struct inode *iq;

	iq = ip->i_freef;
	if (iq)
		iq->i_freeb = &ifreeh;
	ifreeh = iq;
	}
	ip->i_freef = NULL;
	ip->i_freeb = NULL;
	/*
	 * Now to take inode off the hash chain it was on
	 * (initially, or after an iflush, it is on a "hash chain"
	 * consisting entirely of itself, and pointed to by no-one,
	 * but that doesn't matter), and put it on the chain for
	 * its new (ino, dev) pair
	 */
	remque(ip);
	insque(ip, ih);
	ip->i_dev = dev;
	ip->i_fs = fs;
	ip->i_number = ino;
	cacheinval(ip);
	ip->i_flag = ILOCKED;
	ip->i_count++;
	ip->i_lastr = 0;
	bp = bread(dev, itod(ino));
	/*
	 * Check I/O errors
	 */
	if ((bp->b_flags&B_ERROR) != 0) {
		brelse(bp);
		/*
		 * the inode doesn't contain anything useful, so it would
		 * be misleading to leave it on its hash chain.
		 * 'iput' will take care of putting it back on the free list.
		 */
		remque(ip);
		ip->i_forw = ip;
		ip->i_back = ip;
		/*
		 * we also loose its inumber, just in case (as iput
		 * doesn't do that any more) - but as it isn't on its
		 * hash chain, I doubt if this is really necessary .. kre
		 * (probably the two methods are interchangable)
		 */
		ip->i_number = 0;
		iput(ip);
		return(NULL);
	}
	dp = (struct dinode*) bp->b_addr;
	dp += itoo(ino);
	ip->i_ic1 = dp->di_ic1;
	ip->i_flags = dp->di_flags;
	ip->i_ic2 = dp->di_ic2;
	bcopy(dp->di_addr, ip->i_addr, NADDR * sizeof (daddr_t));
	brelse(bp);
	return (ip);
}

/*
 * Convert a pointer to an inode into a reference to an inode.
 *
 * This is basically the internal piece of iget (after the
 * inode pointer is located) but without the test for mounted
 * filesystems.  It is caller's responsibility to check that
 * the inode pointer is valid.
 */
void
igrab (ip)
	register struct inode *ip;
{
	while ((ip->i_flag&ILOCKED) != 0) {
		ip->i_flag |= IWANT;
		sleep((caddr_t)ip, PINOD);
	}
	if (ip->i_count == 0) {		/* ino on free list */
		register struct inode *iq;

		iq = ip->i_freef;
		if (iq)
			iq->i_freeb = ip->i_freeb;
		else
			ifreet = ip->i_freeb;
		*ip->i_freeb = iq;
		ip->i_freef = NULL;
		ip->i_freeb = NULL;
	}
	ip->i_count++;
	ip->i_flag |= ILOCKED;
}

/*
 * Decrement reference count of
 * an inode structure.
 * On the last reference,
 * write the inode out and if necessary,
 * truncate and deallocate the file.
 */
void
iput (ip)
	register struct inode *ip;
{
#ifdef notnow
	/*
	 * This code requires a lot of workarounds, you have to change
	 * lots of places to gratuitously lock just so we can unlock it.
	 * Not worth it.  -- KB
	 */
	if ((ip->i_flag & ILOCKED) == 0)
		panic("iput");
#endif
	IUNLOCK(ip);
	irele(ip);
}

void
irele (ip)
	register struct inode *ip;
{
	if (ip->i_count == 1) {
		ip->i_flag |= ILOCKED;
		if (ip->i_nlink <= 0 && ip->i_fs->fs_ronly == 0) {
			itrunc (ip, (u_long) 0, 0);
			ip->i_mode = 0;
			ip->i_rdev = 0;
			ip->i_flag |= IUPD|ICHG;
			ifree(ip, ip->i_number);
		}
		IUPDAT(ip, &time, &time, 0);
		IUNLOCK(ip);
		ip->i_flag = 0;
		/*
		 * Put the inode on the end of the free list.
		 * Possibly in some cases it would be better to
		 * put the inode at the head of the free list,
		 * (eg: where i_mode == 0 || i_number == 0)
		 * but I will think about that later .. kre
		 * (i_number is rarely 0 - only after an i/o error in iget,
		 * where i_mode == 0, the inode will probably be wanted
		 * again soon for an ialloc, so possibly we should keep it)
		 */
		if (ifreeh) {
			*ifreet = ip;
			ip->i_freeb = ifreet;
		} else {
			ifreeh = ip;
			ip->i_freeb = &ifreeh;
		}
		ip->i_freef = NULL;
		ifreet = &ip->i_freef;
	} else if (!(ip->i_flag & ILOCKED))
		ITIMES(ip, &time, &time);
	ip->i_count--;
}

/*
 * Check accessed and update flags on
 * an inode structure.
 * If any are on, update the inode
 * with the current time.
 * If waitfor set, then must insure
 * i/o order so wait for the write to complete.
 */
void
iupdat (ip, ta, tm, waitfor)
	struct inode *ip;
	struct timeval *ta, *tm;
	int waitfor;
{
	register struct buf *bp;
	register struct dinode *dp;
	register struct inode *tip = ip;

	if ((tip->i_flag & (IUPD|IACC|ICHG|IMOD)) == 0)
		return;
	if (tip->i_fs->fs_ronly)
		return;
	bp = bread(tip->i_dev, itod(tip->i_number));
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return;
	}
	if (tip->i_flag&IACC)
		tip->i_atime = ta->tv_sec;
	if (tip->i_flag&IUPD)
		tip->i_mtime = tm->tv_sec;
	if (tip->i_flag&ICHG)
		tip->i_ctime = time.tv_sec;
	tip->i_flag &= ~(IUPD|IACC|ICHG|IMOD);
	dp = (struct dinode*) bp->b_addr + itoo (tip->i_number);
	dp->di_ic1 = tip->i_ic1;
	dp->di_flags = tip->i_flags;
	dp->di_ic2 = tip->i_ic2;
	bcopy(ip->i_addr, dp->di_addr, NADDR * sizeof (daddr_t));
	if (waitfor && ((ip->i_fs->fs_flags & MNT_ASYNC) == 0))
		bwrite(bp);
	else
		bdwrite(bp);
}

#define	SINGLE	0	/* index of single indirect block */
#define	DOUBLE	1	/* index of double indirect block */
#define	TRIPLE	2	/* index of triple indirect block */

static void
trsingle (ip, bp, last, aflags)
	register struct inode *ip;
	struct buf *bp;
	daddr_t last;
	int aflags;
{
	register const daddr_t *bstart, *bstop;
	const daddr_t *blarray = (const daddr_t*) bp->b_addr;

	bstart = &blarray[NINDIR - 1];
	bstop = &blarray[last];
	for (; bstart > bstop; --bstart)
		if (*bstart)
			free (ip, *bstart);
}

/*
 * Release blocks associated with the inode ip and
 * stored in the indirect block bn.  Blocks are free'd
 * in LIFO order up to (but not including) lastbn.  If
 * level is greater than SINGLE, the block is an indirect
 * block and recursive calls to indirtrunc must be used to
 * cleanse other indirect blocks.
 *
 * NB: triple indirect blocks are untested.
 */
void
indirtrunc (ip, bn, lastbn, level, aflags)
	struct inode *ip;
	daddr_t bn, lastbn;
	int level;
	int aflags;
{
	register struct buf *bp;
	daddr_t nb, last;
	long factor;

	/*
	 * Calculate index in current block of last
	 * block to be kept.  -1 indicates the entire
	 * block so we need not calculate the index.
	 */
	switch (level) {
	default:
	case SINGLE:
		factor = 1;
		break;
	case DOUBLE:
		factor = NINDIR;
		break;
	case TRIPLE:
		factor = NINDIR * NINDIR;
		break;
	}
	last = lastbn;
	if (lastbn > 0)
		last = last / factor;
	/*
	 * Get buffer of block pointers, zero those
	 * entries corresponding to blocks to be free'd,
	 * and update on disk copy first.
	 */
	{
		register daddr_t *bap;
		register struct buf *cpy;

		bp = bread(ip->i_dev, bn);
		if (bp->b_flags&B_ERROR) {
			brelse(bp);
			return;
		}
		cpy = geteblk();
		bcopy (bp->b_addr, cpy->b_addr, DEV_BSIZE);
		bap = (daddr_t*) bp->b_addr;
		bzero((caddr_t)&bap[last + 1],
		    (u_int)(NINDIR - (last + 1)) * sizeof(daddr_t));
		if (aflags & B_SYNC)
			bwrite(bp);
		else
			bawrite(bp);
		bp = cpy;
	}

	/*
	 * Optimized for single indirect blocks, i.e. until a file is
	 * greater than 4K + 256K you don't have to do a mapin/mapout
	 * for every entry.  The mapin/mapout is required since free()
	 * may have to map an item in.  Have to use another routine
	 * since it requires 1K of kernel stack to get around the problem
	 * and that doesn't work well with recursion.
	 */
	if (level == SINGLE)
		trsingle (ip, bp, last, aflags);
	else {
		register daddr_t *bstart, *bstop;

		bstart = (daddr_t*) bp->b_addr;
		bstop = &bstart[last];
		bstart += NINDIR - 1;
		/*
		 * Recursively free totally unused blocks.
		 */
		for (;bstart > bstop;--bstart) {
			nb = *bstart;
			if (nb) {
				indirtrunc(ip,nb,(daddr_t)-1, level-1, aflags);
				free(ip, nb);
			}
		}

		/*
		 * Recursively free last partial block.
		 */
		if (lastbn >= 0) {
			nb = *bstop;
			last = lastbn % factor;
			if (nb != 0)
				indirtrunc(ip, nb, last, level - 1, aflags);
		}
	}
	brelse(bp);
}

/*
 * Truncate the inode ip to at most
 * length size.  Free affected disk
 * blocks -- the blocks of the file
 * are removed in reverse order.
 *
 * NB: triple indirect blocks are untested.
 */
void
itrunc (oip, length, ioflags)
	register struct inode *oip;
	u_long length;
	int ioflags;
{
	daddr_t lastblock;
	register int i;
	register struct inode *ip;
	daddr_t bn, lastiblock[NIADDR];
	struct buf *bp;
	int offset, level;
	struct inode tip;
	int aflags;

	aflags = B_CLRBUF;
	if (ioflags & IO_SYNC)
		aflags |= B_SYNC;

	/*
	 * special hack for pipes, since size for them isn't the size of
	 * the file, it's the amount currently waiting for transfer.  It's
	 * unclear that this will work, though, because pipes can (although
	 * rarely do) get bigger than MAXPIPSIZ.  Don't think it worked
	 * in V7 either, I don't really understand what's going on.
	 */
	if (oip->i_flag & IPIPE)
		oip->i_size = MAXPIPSIZ;
	else if (oip->i_size == length)
		goto updret;

	/*
	 * Lengthen the size of the file. We must ensure that the
	 * last byte of the file is allocated. Since the smallest
	 * value of osize is 0, length will be at least 1.
	 */
	if (oip->i_size < length) {
		bn = bmap(oip, lblkno(length - 1), B_WRITE, aflags);
		if (u.u_error || bn < 0)
			return;
		oip->i_size = length;
		goto doquotaupd;
	}

	/*
	 * Calculate index into inode's block list of
	 * last direct and indirect blocks (if any)
	 * which we want to keep.  Lastblock is -1 when
	 * the file is truncated to 0.
	 */
	lastblock = lblkno(length + DEV_BSIZE - 1) - 1;
	lastiblock[SINGLE] = lastblock - NDADDR;
	lastiblock[DOUBLE] = lastiblock[SINGLE] - NINDIR;
	lastiblock[TRIPLE] = lastiblock[DOUBLE] - NINDIR * NINDIR;
	/*
	 * Update the size of the file. If the file is not being
	 * truncated to a block boundry, the contents of the
	 * partial block following the end of the file must be
	 * zero'ed in case it ever become accessable again because
	 * of subsequent file growth.
	 */
	offset = blkoff(length);
	if (offset) {
		bn = bmap(oip, lblkno(length), B_WRITE, aflags);
		if (u.u_error || bn < 0)
			return;
		bp = bread(oip->i_dev, bn);
		if (bp->b_flags & B_ERROR) {
			u.u_error = EIO;
			brelse(bp);
			return;
		}
		bzero (bp->b_addr + offset, (u_int) (DEV_BSIZE - offset));
		bdwrite(bp);
	}
	/*
	 * Update file and block pointers
	 * on disk before we start freeing blocks.
	 * If we crash before free'ing blocks below,
	 * the blocks will be returned to the free list.
	 * lastiblock values are also normalized to -1
	 * for calls to indirtrunc below.
	 */
	tip = *oip;
	oip->i_size = length;
	for (level = TRIPLE; level >= SINGLE; level--)
		if (lastiblock[level] < 0) {
			oip->i_ib[level] = 0;
			lastiblock[level] = -1;
		}
	for (i = NDADDR - 1; i > lastblock; i--)
		oip->i_db[i] = 0;

	/*
	 * Indirect blocks first.
	 */
	ip = &tip;
	for (level = TRIPLE; level >= SINGLE; level--) {
		bn = ip->i_ib[level];
		if (bn != 0) {
			indirtrunc(ip, bn, lastiblock[level], level, aflags);
			if (lastiblock[level] < 0) {
				ip->i_ib[level] = 0;
				free(ip, bn);
			}
		}
		if (lastiblock[level] >= 0)
			goto done;
	}

	/*
	 * All whole direct blocks.
	 */
	for (i = NDADDR - 1; i > lastblock; i--) {
		bn = ip->i_db[i];
		if (bn == 0)
			continue;
		ip->i_db[i] = 0;
		free(ip, bn);
	}
	if (lastblock < 0)
		goto done;

done:
#ifdef DIAGNOSTIC
/* BEGIN PARANOIA */
	for (level = SINGLE; level <= TRIPLE; level++)
		if (ip->i_ib[level] != oip->i_ib[level])
			panic("itrunc1");
	for (i = 0; i < NDADDR; i++)
		if (ip->i_db[i] != oip->i_db[i])
			panic("itrunc2");
/* END PARANOIA */
#endif

doquotaupd:
updret:
	oip->i_flag |= ICHG|IUPD;
	iupdat(oip, &time, &time, 1);
}

/*
 * Remove any inodes in the inode cache belonging to dev.
 *
 * There should not be any active ones, return error if any are found
 * (nb: this is a user error, not a system err)
 *
 * Also, count the references to dev by block devices - this really
 * has nothing to do with the object of the procedure, but as we have
 * to scan the inode table here anyway, we might as well get the
 * extra benefit.
 *
 * this is called from sumount() when dev is being unmounted
 */
int
iflush (dev)
	dev_t dev;
{
	register struct inode *ip;
	register int open = 0;

	for (ip = inode; ip < inode+NINODE; ip++) {
		if (ip->i_dev == dev)
			if (ip->i_count)
				return(-1);
			else {
				remque(ip);
				ip->i_forw = ip;
				ip->i_back = ip;
				/*
				 * as i_count == 0, the inode was on the free
				 * list already, just leave it there, it will
				 * fall off the bottom eventually. We could
				 * perhaps move it to the head of the free
				 * list, but as umounts are done so
				 * infrequently, we would gain very little,
				 * while making the code bigger.
				 */
			}
		else if (ip->i_count && (ip->i_mode&IFMT)==IFBLK &&
		    ip->i_rdev == dev)
			open++;
	}
	return (open);
}

/*
 * Lock an inode. If its already locked, set the WANT bit and sleep.
 */
void
ilock(ip)
	register struct inode *ip;
{
	ILOCK(ip);
}

/*
 * Unlock an inode.  If WANT bit is on, wakeup.
 */
void
iunlock(ip)
	register struct inode *ip;
{
	IUNLOCK(ip);
}
