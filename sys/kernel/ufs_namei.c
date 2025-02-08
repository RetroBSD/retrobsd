/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dir.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/mount.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/namei.h>
#include <sys/proc.h>

int dirchk = 0;

/*
 * Structures associated with name cacheing.
 */
#define NCHHASH         16  /* size of hash table */

#if ((NCHHASH)&((NCHHASH)-1)) != 0
#define NHASH(h, i, d)  ((unsigned)((h) + (i) + 13 * (int)(d)) % (NCHHASH))
#else
#define NHASH(h, i, d)  ((unsigned)((h) + (i) + 13 * (int)(d)) & ((NCHHASH)-1))
#endif

union nchash {
    union   nchash *nch_head[2];
    struct  namecache *nch_chain[2];
} nchash[NCHHASH];

#define nch_forw    nch_chain[0]
#define nch_back    nch_chain[1]

struct  namecache *nchhead, **nchtail;  /* LRU chain pointers */
u_int nextinodeid;                      /* unique id generator */
struct inode *rootdir;                  /* pointer to inode of root directory */
struct nchstats nchstats;               /* cache effectiveness statistics */

static void
dirbad (struct inode *ip, off_t offset, char *how)
{
    printf ("%s: bad dir I=%u off %ld: %s\n",
        ip->i_fs->fs_fsmnt, ip->i_number, offset, how);
}

/*
 * Return buffer with contents of block "offset"
 * from the beginning of directory "ip".  If "res"
 * is non-zero, fill it in with a pointer to the
 * remaining space in the directory.
 */
static struct buf *
blkatoff(struct inode *ip, off_t offset, char **res)
{
    daddr_t lbn = lblkno(offset);
    register struct buf *bp;
    daddr_t bn;
    char *junk;

    bn = bmap(ip, lbn, B_READ, 0);
    if (u.u_error)
        return (0);
    if (bn == (daddr_t)-1) {
        dirbad(ip, offset, "hole in dir");
        return (0);
    }
    bp = bread(ip->i_dev, bn);
    if (bp->b_flags & B_ERROR) {
        brelse(bp);
        return (0);
    }
    junk = (caddr_t) bp->b_addr;
    if (res)
        *res = junk + (u_int)blkoff(offset);
    return (bp);
}

/*
 * Do consistency checking on a directory entry:
 *  record length must be multiple of 4
 *  entry must fit in rest of its DIRBLKSIZ block
 *  record must be large enough to contain entry
 *  name is not longer than MAXNAMLEN
 *  name must be as long as advertised, and null terminated
 */
static int
dirbadentry (struct direct *ep, int entryoffsetinblock)
{
    register int i;

    if ((ep->d_reclen & 0x3) != 0 ||
        ep->d_reclen > DIRBLKSIZ - (entryoffsetinblock & (DIRBLKSIZ - 1)) ||
        ep->d_reclen < DIRSIZ(ep) || ep->d_namlen > MAXNAMLEN)
        return (1);
    for (i = 0; i < ep->d_namlen; i++)
        if (ep->d_name[i] == '\0')
            return (1);
    return (ep->d_name[i]);
}

/*
 * Convert a pathname into a pointer to a locked inode.
 * This is a very central and rather complicated routine.
 * If the file system is not maintained in a strict tree hierarchy,
 * this can result in a deadlock situation (see comments in code below).
 *
 * The flag argument is LOOKUP, CREATE, or DELETE depending on whether
 * the name is to be looked up, created, or deleted. When CREATE or
 * DELETE is specified, information usable in creating or deleteing a
 * directory entry is also calculated. If flag has LOCKPARENT or'ed
 * into it and the target of the pathname exists, namei returns both
 * the target and its parent directory locked. When creating and
 * LOCKPARENT is specified, the target may not be ".".  When deleting
 * and LOCKPARENT is specified, the target may be ".", but the caller
 * must check to insure it does an irele and iput instead of two iputs.
 *
 * The FOLLOW flag is set when symbolic links are to be followed
 * when they occur at the end of the name translation process.
 * Symbolic links are always followed for all other pathname
 * components other than the last.
 *
 * Name caching works as follows:
 *
 * Names found by directory scans are retained in a cache
 * for future reference.  It is managed LRU, so frequently
 * used names will hang around.  Cache is indexed by hash value
 * obtained from (ino,dev,name) where ino & dev refer to the
 * directory containing name.
 *
 * For simplicity (and economy of storage), names longer than
 * a maximum length of NCHNAMLEN are not cached; they occur
 * infrequently in any case, and are almost never of interest.
 *
 * Upon reaching the last segment of a path, if the reference
 * is for DELETE, or NOCACHE is set (rewrite), and the
 * name is located in the cache, it will be dropped.
 *
 * Overall outline of namei:
 *
 *  copy in name
 *  get starting directory
 * dirloop:
 *  check accessibility of directory
 * dirloop2:
 *  copy next component of name to ndp->ni_dent
 *  handle degenerate case where name is null string
 *  look for name in cache, if found, then if at end of path
 *    and deleting or creating, drop it, else to haveino
 *  search for name in directory, to found or notfound
 * notfound:
 *  if creating, return locked directory, leaving info on avail. slots
 *  else return error
 * found:
 *  if at end of path and deleting, return information to allow delete
 *  if at end of path and rewriting (CREATE and LOCKPARENT), lock target
 *    inode and return info to allow rewrite
 *  if .. and on mounted filesys, look in mount table for parent
 *  if not at end, add name to cache; if at end and neither creating
 *    nor deleting, add name to cache
 * haveino:
 *  if symbolic link, massage name in buffer and continue at dirloop
 *  if more components of name, do next level at dirloop
 *  return the answer as locked inode
 *
 * NOTE: (LOOKUP | LOCKPARENT) currently returns the parent inode,
 *   but unlocked.
 */
struct inode *
namei (struct nameidata *ndp)
{
    register char *cp;          /* pointer into pathname argument */
/* these variables refer to things which must be freed or unlocked */
    struct inode *dp = 0;       /* the directory we are searching */
    struct namecache *ncp = 0;  /* cache slot for entry */
    struct fs *fs;              /* file system that directory is in */
    struct buf *bp = 0;         /* a buffer of directory entries */
    struct direct *ep;          /* the current directory entry */
    int entryoffsetinblock = 0; /* offset of ep in bp's buffer */
/* these variables hold information about the search for a slot */
    enum {NONE, COMPACT, FOUND} slotstatus;
    off_t slotoffset = -1;      /* offset of area with free space */
    int slotsize = 0;           /* size of area at slotoffset */
    int slotfreespace = 0;      /* amount of space free in slot */
    int slotneeded = 0;         /* size of the entry we're seeking */
/* */
    int numdirpasses;           /* strategy for directory search */
    off_t endsearch;            /* offset to end directory search */
    off_t prevoff = 0;          /* ndp->ni_offset of previous entry */
    int nlink = 0;              /* number of symbolic links taken */
    struct inode *pdp;          /* saved dp during symlink work */
    register int i;
    int error;
    int lockparent;
    int docache;                /* == 0 do not cache last component */
    int makeentry;              /* != 0 if name to be added to cache */
    unsigned hash;              /* value of name hash for entry */
    union nchash *nhp = 0;      /* cache chain head for entry */
    int isdotdot;               /* != 0 if current name is ".." */
    int flag;                   /* op ie, LOOKUP, CREATE, or DELETE */
    off_t enduseful;            /* pointer past last used dir slot */
    char    path[MAXPATHLEN];   /* current path */

    lockparent = ndp->ni_nameiop & LOCKPARENT;
    docache = (ndp->ni_nameiop & NOCACHE) ^ NOCACHE;
    flag = ndp->ni_nameiop &~ (LOCKPARENT|NOCACHE|FOLLOW);
    if (flag == DELETE || lockparent)
        docache = 0;
    /*
     * Copy the name into the buffer.
     */
    error = copystr (ndp->ni_dirp, path, MAXPATHLEN, (u_int*) 0);
    if (error) {
        u.u_error = error;
        goto retNULL;
    }

    /*
     * Get starting directory.
     */
    cp = path;
    if (*cp == '/') {
        while (*cp == '/')
            cp++;
        if ((dp = u.u_rdir) == NULL)
            dp = rootdir;
    } else
        dp = u.u_cdir;
    fs = dp->i_fs;
    ILOCK(dp);
    dp->i_count++;
    ndp->ni_endoff = 0;

    /*
     * We come to dirloop to search a new directory.
     * The directory must be locked so that it can be
     * iput, and fs must be already set to dp->i_fs.
     */
dirloop:
    /*
     * Check accessibility of directory.
     */
    if ((dp->i_mode&IFMT) != IFDIR) {
        u.u_error = ENOTDIR;
        goto bad;
    }
    if (access(dp, IEXEC))
        goto bad;

dirloop2:
    /*
     * Copy next component of name to ndp->ni_dent.
     */
    hash = 0;
    for (i = 0; *cp != 0 && *cp != '/'; cp++) {
        if (i >= MAXNAMLEN) {
            u.u_error = ENAMETOOLONG;
            goto bad;
        }
        if (*cp & 0200)
            if ((*cp&0377) == ('/'|0200) || flag != DELETE) {
                u.u_error = EINVAL;
                goto bad;
            }
        ndp->ni_dent.d_name[i++] = *cp;
        hash += (unsigned char)*cp * i;
    }
    ndp->ni_dent.d_namlen = i;
    ndp->ni_dent.d_name[i] = '\0';
    isdotdot = (i == 2 &&
        ndp->ni_dent.d_name[0] == '.' && ndp->ni_dent.d_name[1] == '.');
    makeentry = 1;
    if (*cp == '\0' && docache == 0)
        makeentry = 0;

    /*
     * Check for degenerate name (e.g. / or "")
     * which is a way of talking about a directory,
     * e.g. like "/." or ".".
     */
    if (ndp->ni_dent.d_name[0] == '\0') {
        if (flag != LOOKUP || lockparent) {
            u.u_error = EISDIR;
            goto bad;
        }
        goto retDP;
     }

    /*
     * We now have a segment name to search for, and a directory to search.
     *
     * Before tediously performing a linear scan of the directory,
     * check the name cache to see if the directory/name pair
     * we are looking for is known already.  We don't do this
     * if the segment name is long, simply so the cache can avoid
     * holding long names (which would either waste space, or
     * add greatly to the complexity).
     */
    if (ndp->ni_dent.d_namlen > NCHNAMLEN) {
        nchstats.ncs_long++;
        makeentry = 0;
    } else {
        nhp = &nchash[NHASH(hash, dp->i_number, dp->i_dev)];
        for (ncp = nhp->nch_forw; ncp != (struct namecache *)nhp;
            ncp = ncp->nc_forw) {
            if (ncp->nc_ino == dp->i_number &&
                ncp->nc_dev == dp->i_dev &&
                ncp->nc_nlen == ndp->ni_dent.d_namlen &&
                !bcmp(ncp->nc_name, ndp->ni_dent.d_name,
                (unsigned)ncp->nc_nlen))
                break;
        }
        if (ncp == (struct namecache *)nhp) {
            nchstats.ncs_miss++;
            ncp = NULL;
        } else {
            if (ncp->nc_id != ncp->nc_ip->i_id)
                nchstats.ncs_falsehits++;
            else if (!makeentry)
                nchstats.ncs_badhits++;
            else {
                /*
                 * move this slot to end of LRU
                 * chain, if not already there
                 */
                if (ncp->nc_nxt) {
                    /* remove from LRU chain */
                    *ncp->nc_prev = ncp->nc_nxt;
                    ncp->nc_nxt->nc_prev = ncp->nc_prev;

                    /* and replace at end of it */
                    ncp->nc_nxt = NULL;
                    ncp->nc_prev = nchtail;
                    *nchtail = ncp;
                    nchtail = &ncp->nc_nxt;
                }

                /*
                 * Get the next inode in the path.
                 * See comment above other `IUNLOCK' code for
                 * an explaination of the locking protocol.
                 */
                pdp = dp;
                if (!isdotdot || dp != u.u_rdir)
                    dp = ncp->nc_ip;
                if (dp == NULL)
                    panic("namei: null cache ino");
                if (pdp == dp)
                    dp->i_count++;
                else if (isdotdot) {
                    IUNLOCK(pdp);
                    igrab(dp);
                } else {
                    igrab(dp);
                    IUNLOCK(pdp);
                }

                /*
                 * Verify that the inode that we got
                 * did not change while we were waiting
                 * for it to be locked.
                 */
                if (ncp->nc_id != ncp->nc_ip->i_id) {
                    iput(dp);
                    ILOCK(pdp);
                    dp = pdp;
                    nchstats.ncs_falsehits++;
                } else {
                    ndp->ni_dent.d_ino = dp->i_number;
                    /* ni_dent.d_reclen is garbage ... */
                    nchstats.ncs_goodhits++;
                    goto haveino;
                }
            }

            /*
             * Last component and we are renaming or deleting,
             * the cache entry is invalid, or otherwise don't
             * want cache entry to exist.
             */
            /* remove from LRU chain */
            *ncp->nc_prev = ncp->nc_nxt;
            if (ncp->nc_nxt)
                ncp->nc_nxt->nc_prev = ncp->nc_prev;
            else
                nchtail = ncp->nc_prev;
            remque(ncp);        /* remove from hash chain */
            /* insert at head of LRU list (first to grab) */
            ncp->nc_nxt = nchhead;
            ncp->nc_prev = &nchhead;
            nchhead->nc_prev = &ncp->nc_nxt;
            nchhead = ncp;
            /* and make a dummy hash chain */
            ncp->nc_forw = ncp;
            ncp->nc_back = ncp;
            ncp = NULL;
        }
    }

    /*
     * Suppress search for slots unless creating
     * file and at end of pathname, in which case
     * we watch for a place to put the new file in
     * case it doesn't already exist.
     */
    slotstatus = FOUND;
    if (flag == CREATE && *cp == 0) {
        slotstatus = NONE;
        slotfreespace = 0;
        slotneeded = DIRSIZ(&ndp->ni_dent);
    }
    /*
     * If this is the same directory that this process
     * previously searched, pick up where we last left off.
     * We cache only lookups as these are the most common
     * and have the greatest payoff. Caching CREATE has little
     * benefit as it usually must search the entire directory
     * to determine that the entry does not exist. Caching the
     * location of the last DELETE has not reduced profiling time
     * and hence has been removed in the interest of simplicity.
     */
    if (flag != LOOKUP || dp->i_number != u.u_ncache.nc_inumber ||
        dp->i_dev != u.u_ncache.nc_dev) {
            ndp->ni_offset = 0;
            numdirpasses = 1;
    } else {
        if (u.u_ncache.nc_prevoffset > dp->i_size)
            u.u_ncache.nc_prevoffset = 0;
        ndp->ni_offset = u.u_ncache.nc_prevoffset;
        entryoffsetinblock = blkoff(ndp->ni_offset);
        if (entryoffsetinblock != 0) {
            bp = blkatoff(dp, ndp->ni_offset, (char **)0);
            if (bp == 0)
                goto bad;
        }
        numdirpasses = 2;
        nchstats.ncs_2passes++;
    }
    endsearch = roundup(dp->i_size, DIRBLKSIZ);
    enduseful = 0;

searchloop:
    while (ndp->ni_offset < endsearch) {
        /*
         * If offset is on a block boundary,
         * read the next directory block.
         * Release previous if it exists.
         */
        if (blkoff(ndp->ni_offset) == 0) {
            if (bp != NULL) {
                brelse(bp);
            }
            bp = blkatoff(dp, ndp->ni_offset, (char **)0);
            if (bp == 0)
                goto bad;
            entryoffsetinblock = 0;
        }
        /*
         * If still looking for a slot, and at a DIRBLKSIZE
         * boundary, have to start looking for free space again.
         */
        if (slotstatus == NONE &&
            (entryoffsetinblock&(DIRBLKSIZ-1)) == 0) {
            slotoffset = -1;
            slotfreespace = 0;
        }
        /*
         * Get pointer to next entry.
         * Full validation checks are slow, so we only check
         * enough to insure forward progress through the
         * directory. Complete checks can be run by patching
         * "dirchk" to be true.
         */
        ep = (struct direct*) ((caddr_t) bp->b_addr + entryoffsetinblock);
        if (ep->d_reclen == 0 ||
            (dirchk && dirbadentry (ep, entryoffsetinblock))) {
            dirbad(dp, ndp->ni_offset, "mangled entry");
            i = DIRBLKSIZ - (entryoffsetinblock & (DIRBLKSIZ - 1));
            ndp->ni_offset += i;
            entryoffsetinblock += i;
            continue;
        }

        /*
         * If an appropriate sized slot has not yet been found,
         * check to see if one is available. Also accumulate space
         * in the current block so that we can determine if
         * compaction is viable.
         */
        if (slotstatus != FOUND) {
            int size = ep->d_reclen;

            if (ep->d_ino != 0)
                size -= DIRSIZ(ep);
            if (size > 0) {
                if (size >= slotneeded) {
                    slotstatus = FOUND;
                    slotoffset = ndp->ni_offset;
                    slotsize = ep->d_reclen;
                } else if (slotstatus == NONE) {
                    slotfreespace += size;
                    if (slotoffset == -1)
                        slotoffset = ndp->ni_offset;
                    if (slotfreespace >= slotneeded) {
                        slotstatus = COMPACT;
                        slotsize = ndp->ni_offset +
                              ep->d_reclen - slotoffset;
                    }
                }
            }
        }

        /*
         * Check for a name match.
         */
        if (ep->d_ino) {
            if (ep->d_namlen == ndp->ni_dent.d_namlen &&
                !bcmp(ndp->ni_dent.d_name, ep->d_name,
                (unsigned)ep->d_namlen))
                goto found;
        }
        prevoff = ndp->ni_offset;
        ndp->ni_offset += ep->d_reclen;
        entryoffsetinblock += ep->d_reclen;
        if (ep->d_ino)
            enduseful = ndp->ni_offset;
    }
/* notfound: */
    /*
     * If we started in the middle of the directory and failed
     * to find our target, we must check the beginning as well.
     */
    if (numdirpasses == 2) {
        numdirpasses--;
        ndp->ni_offset = 0;
        endsearch = u.u_ncache.nc_prevoffset;
        goto searchloop;
    }
    /*
     * If creating, and at end of pathname and current
     * directory has not been removed, then can consider
     * allowing file to be created.
     */
    if (flag == CREATE && *cp == 0 && dp->i_nlink != 0) {
        /*
         * Access for write is interpreted as allowing
         * creation of files in the directory.
         */
        if (access(dp, IWRITE))
            goto bad;
        /*
         * Return an indication of where the new directory
         * entry should be put.  If we didn't find a slot,
         * then set ndp->ni_count to 0 indicating that the new
         * slot belongs at the end of the directory. If we found
         * a slot, then the new entry can be put in the range
         * [ndp->ni_offset .. ndp->ni_offset + ndp->ni_count)
         */
        if (slotstatus == NONE) {
            ndp->ni_offset = roundup(dp->i_size, DIRBLKSIZ);
            ndp->ni_count = 0;
            enduseful = ndp->ni_offset;
        } else {
            ndp->ni_offset = slotoffset;
            ndp->ni_count = slotsize;
            if (enduseful < slotoffset + slotsize)
                enduseful = slotoffset + slotsize;
        }
        ndp->ni_endoff = roundup(enduseful, DIRBLKSIZ);
        dp->i_flag |= IUPD|ICHG;
        if (bp) {
            brelse(bp);
        }
        /*
         * We return with the directory locked, so that
         * the parameters we set up above will still be
         * valid if we actually decide to do a direnter().
         * We return NULL to indicate that the entry doesn't
         * currently exist, leaving a pointer to the (locked)
         * directory inode in ndp->ni_pdir.
         */
        ndp->ni_pdir = dp;
        goto retNULL;
    }
    u.u_error = ENOENT;
    goto bad;
found:
    if (numdirpasses == 2)
        nchstats.ncs_pass2++;
    /*
     * Check that directory length properly reflects presence
     * of this entry.
     */
    if (entryoffsetinblock + DIRSIZ(ep) > dp->i_size) {
        dirbad(dp, ndp->ni_offset, "i_size too small");
        dp->i_size = entryoffsetinblock + DIRSIZ(ep);
        dp->i_flag |= IUPD|ICHG;
    }

    /*
     * Found component in pathname.
     * If the final component of path name, save information
     * in the cache as to where the entry was found.
     */
    if (*cp == '\0' && flag == LOOKUP) {
        u.u_ncache.nc_prevoffset = ndp->ni_offset &~ (DIRBLKSIZ - 1);
        u.u_ncache.nc_inumber = dp->i_number;
        u.u_ncache.nc_dev = dp->i_dev;
    }
    /*
     * Save directory entry's inode number and reclen in ndp->ni_dent,
     * and release directory buffer.
     */
    ndp->ni_dent.d_ino = ep->d_ino;
    ndp->ni_dent.d_reclen = ep->d_reclen;
    brelse(bp);
    bp = NULL;

    /*
     * If deleting, and at end of pathname, return
     * parameters which can be used to remove file.
     * If the lockparent flag isn't set, we return only
     * the directory (in ndp->ni_pdir), otherwise we go
     * on and lock the inode, being careful with ".".
     */
    if (flag == DELETE && *cp == 0) {
        /*
         * Write access to directory required to delete files.
         */
        if (access(dp, IWRITE))
            goto bad;
        ndp->ni_pdir = dp;      /* for dirremove() */
        /*
         * Return pointer to current entry in ndp->ni_offset,
         * and distance past previous entry (if there
         * is a previous entry in this block) in ndp->ni_count.
         * Save directory inode pointer in ndp->ni_pdir for dirremove().
         */
        if ((ndp->ni_offset&(DIRBLKSIZ-1)) == 0)
            ndp->ni_count = 0;
        else
            ndp->ni_count = ndp->ni_offset - prevoff;
        if (lockparent) {
            if (dp->i_number == ndp->ni_dent.d_ino)
                dp->i_count++;
            else {
                dp = iget(dp->i_dev, fs, ndp->ni_dent.d_ino);
                if (dp == NULL) {
                    iput(ndp->ni_pdir);
                    goto bad;
                }
                /*
                 * If directory is "sticky", then user must own
                 * the directory, or the file in it, else he
                 * may not delete it (unless he's root). This
                 * implements append-only directories.
                 */
                if ((ndp->ni_pdir->i_mode & ISVTX) &&
                    u.u_uid != 0 &&
                    u.u_uid != ndp->ni_pdir->i_uid &&
                    dp->i_uid != u.u_uid) {
                    iput(ndp->ni_pdir);
                    u.u_error = EPERM;
                    goto bad;
                }
            }
        }
        goto retDP;
    }

    /*
     * Special handling for ".." allowing chdir out of mounted
     * file system: indirect .. in root inode to reevaluate
     * in directory file system was mounted on.
     */
    if (isdotdot) {
        if (dp == u.u_rdir) {
            ndp->ni_dent.d_ino = dp->i_number;
            makeentry = 0;
        } else if (ndp->ni_dent.d_ino == ROOTINO &&
            dp->i_number == ROOTINO) {
            register struct mount *mp;
            register dev_t d;

            d = dp->i_dev;
            for (mp = &mount[1]; mp < &mount[NMOUNT]; mp++)
                if (mp->m_inodp && mp->m_dev == d) {
                    iput(dp);
                    dp = mp->m_inodp;
                    ILOCK(dp);
                    dp->i_count++;
                    fs = dp->i_fs;
                    cp -= 2;    /* back over .. */
                    goto dirloop2;
                }
        }
    }

    /*
     * If rewriting (rename), return the inode and the
     * information required to rewrite the present directory
     * Must get inode of directory entry to verify it's a
     * regular file, or empty directory.
     */
    if ((flag == CREATE && lockparent) && *cp == 0) {
        if (access(dp, IWRITE))
            goto bad;
        ndp->ni_pdir = dp;      /* for dirrewrite() */
        /*
         * Careful about locking second inode.
         * This can only occur if the target is ".".
         */
        if (dp->i_number == ndp->ni_dent.d_ino) {
            u.u_error = EISDIR;     /* XXX */
            goto bad;
        }
        dp = iget(dp->i_dev, fs, ndp->ni_dent.d_ino);
        if (dp == NULL) {
            iput(ndp->ni_pdir);
            goto bad;
        }
        goto retDP;
    }

    /*
     * Check for symbolic link, which may require us to massage the
     * name before we continue translation.  We do not `iput' the
     * directory because we may need it again if the symbolic link
     * is relative to the current directory.  Instead we save it
     * unlocked as "pdp".  We must get the target inode before unlocking
     * the directory to insure that the inode will not be removed
     * before we get it.  We prevent deadlock by always fetching
     * inodes from the root, moving down the directory tree. Thus
     * when following backward pointers ".." we must unlock the
     * parent directory before getting the requested directory.
     * There is a potential race condition here if both the current
     * and parent directories are removed before the `iget' for the
     * inode associated with ".." returns.  We hope that this occurs
     * infrequently since we cannot avoid this race condition without
     * implementing a sophisticated deadlock detection algorithm.
     * Note also that this simple deadlock detection scheme will not
     * work if the file system has any hard links other than ".."
     * that point backwards in the directory structure.
     */
    pdp = dp;
    if (isdotdot) {
        IUNLOCK(pdp);   /* race to get the inode */
        dp = iget(dp->i_dev, fs, ndp->ni_dent.d_ino);
        if (dp == NULL)
            goto bad2;
    } else if (dp->i_number == ndp->ni_dent.d_ino) {
        dp->i_count++;  /* we want ourself, ie "." */
    } else {
        dp = iget(dp->i_dev, fs, ndp->ni_dent.d_ino);
        IUNLOCK(pdp);
        if (dp == NULL)
            goto bad2;
    }

    /*
     * Insert name into cache if appropriate.
     */
    if (makeentry) {
        if (ncp != NULL)
            panic("namei: duplicating cache");
        /*
         * Free the cache slot at head of lru chain.
         */
        ncp = nchhead;
        if (ncp) {
            /* remove from lru chain */
            *ncp->nc_prev = ncp->nc_nxt;
            if (ncp->nc_nxt)
                ncp->nc_nxt->nc_prev = ncp->nc_prev;
            else
                nchtail = ncp->nc_prev;
            remque(ncp);        /* remove from old hash chain */
            /* grab the inode we just found */
            ncp->nc_ip = dp;
            /* fill in cache info */
            ncp->nc_ino = pdp->i_number;    /* parents inum */
            ncp->nc_dev = pdp->i_dev;   /* & device */
            ncp->nc_idev = dp->i_dev;   /* our device */
            ncp->nc_id = dp->i_id;      /* identifier */
            ncp->nc_nlen = ndp->ni_dent.d_namlen;
            bcopy(ndp->ni_dent.d_name, ncp->nc_name,
                (unsigned)ncp->nc_nlen);
            /* link at end of lru chain */
            ncp->nc_nxt = NULL;
            ncp->nc_prev = nchtail;
            *nchtail = ncp;
            nchtail = &ncp->nc_nxt;
            /* and insert on hash chain */
            insque(ncp, nhp);
        }
    }

haveino:
    fs = dp->i_fs;

    /*
     * Check for symbolic link
     */
    if ((dp->i_mode & IFMT) == IFLNK &&
        ((ndp->ni_nameiop & FOLLOW) || *cp == '/')) {
        u_int pathlen = strlen(cp) + 1;

        if (dp->i_size + pathlen >= MAXPATHLEN - 1) {
            u.u_error = ENAMETOOLONG;
            goto bad2;
        }
        if (++nlink > MAXSYMLINKS) {
            u.u_error = ELOOP;
            goto bad2;
        }

        bp = bread(dp->i_dev, bmap(dp, (daddr_t)0, B_READ, 0));
        if (bp->b_flags & B_ERROR) {
            brelse(bp);
            bp = NULL;
            goto bad2;
        }
        /*
         * Shift the rest of path further down the buffer, then
         * copy link path into the first part of the buffer.
         */
        bcopy(cp, path + (u_int)dp->i_size, pathlen);
        bcopy(bp->b_addr, path, (u_int)dp->i_size);
        brelse(bp);
        bp = NULL;
        cp = path;
        iput(dp);
        if (*cp == '/') {
            irele(pdp);
            while (*cp == '/')
                cp++;
            if ((dp = u.u_rdir) == NULL)
                dp = rootdir;
            ILOCK(dp);
            dp->i_count++;
        } else {
            dp = pdp;
            ILOCK(dp);
        }
        fs = dp->i_fs;
        goto dirloop;
    }

    /*
     * Not a symbolic link.  If more pathname,
     * continue at next component, else return.
     */
    if (*cp == '/') {
        while (*cp == '/')
            cp++;
        irele(pdp);
        goto dirloop;
    }
    if (lockparent)
        ndp->ni_pdir = pdp;
    else
        irele(pdp);
retDP:
    ndp->ni_ip = dp;
    return (dp);

bad2:
    irele(pdp);
bad:
    if (bp) {
        brelse(bp);
    }
    if (dp)
        iput(dp);
retNULL:
    ndp->ni_ip = NULL;
    return (NULL);
}

/*
 * Write a directory entry after a call to namei, using the parameters
 * which it left in the u. area.  The argument ip is the inode which
 * the new directory entry will refer to.  The u. area field ndp->ni_pdir is
 * a pointer to the directory to be written, which was left locked by
 * namei.  Remaining parameters (ndp->ni_offset, ndp->ni_count) indicate
 * how the space for the new entry is to be gotten.
 */
int
direnter(struct inode *ip, struct nameidata *ndp)
{
    register struct direct *ep, *nep;
    register struct inode *dp = ndp->ni_pdir;
    struct buf *bp;
    int loc, spacefree, error = 0;
    u_int dsize;
    int newentrysize;
    char *dirbuf;

    ndp->ni_dent.d_ino = ip->i_number;
    newentrysize = DIRSIZ(&ndp->ni_dent);
    if (ndp->ni_count == 0) {
        /*
         * If ndp->ni_count is 0, then namei could find no space in the
         * directory. In this case ndp->ni_offset will be on a directory
         * block boundary and we will write the new entry into a fresh
         * block.
         */
        if (ndp->ni_offset&(DIRBLKSIZ-1))
            panic("wdir: newblk");
        ndp->ni_dent.d_reclen = DIRBLKSIZ;
        error = rdwri (UIO_WRITE, dp, (caddr_t)&ndp->ni_dent,
                newentrysize, ndp->ni_offset,
                IO_UNIT|IO_SYNC, (int *)0);
        dp->i_size = roundup(dp->i_size, DIRBLKSIZ);
        iput(dp);
        return (error);
    }

    /*
     * If ndp->ni_count is non-zero, then namei found space for the new
     * entry in the range ndp->ni_offset to ndp->ni_offset + ndp->ni_count.
     * in the directory.  To use this space, we may have to compact
     * the entries located there, by copying them together towards
     * the beginning of the block, leaving the free space in
     * one usable chunk at the end.
     */

    /*
     * Increase size of directory if entry eats into new space.
     * This should never push the size past a new multiple of
     * DIRBLKSIZE.
     *
     * N.B. - THIS IS AN ARTIFACT OF 4.2 AND SHOULD NEVER HAPPEN.
     */
    if (ndp->ni_offset + ndp->ni_count > dp->i_size)
        dp->i_size = ndp->ni_offset + ndp->ni_count;
    /*
     * Get the block containing the space for the new directory
     * entry.  Should return error by result instead of u.u_error.
     */
    bp = blkatoff(dp, ndp->ni_offset, (char **)&dirbuf);
    if (bp == 0) {
        iput(dp);
        return (u.u_error);
    }
    /*
     * Find space for the new entry.  In the simple case, the
     * entry at offset base will have the space.  If it does
     * not, then namei arranged that compacting the region
     * ndp->ni_offset to ndp->ni_offset+ndp->ni_count would yield the space.
     */
    ep = (struct direct *)dirbuf;
    dsize = DIRSIZ(ep);
    spacefree = ep->d_reclen - dsize;
    for (loc = ep->d_reclen; loc < ndp->ni_count; ) {
        nep = (struct direct *)(dirbuf + loc);
        if (ep->d_ino) {
            /* trim the existing slot */
            ep->d_reclen = dsize;
            ep = (struct direct *)((char *)ep + dsize);
        } else {
            /* overwrite; nothing there; header is ours */
            spacefree += dsize;
        }
        dsize = DIRSIZ(nep);
        spacefree += nep->d_reclen - dsize;
        loc += nep->d_reclen;
        bcopy((caddr_t)nep, (caddr_t)ep, dsize);
    }
    /*
     * Update the pointer fields in the previous entry (if any),
     * copy in the new entry, and write out the block.
     */
    if (ep->d_ino == 0) {
        if (spacefree + dsize < newentrysize)
            panic("wdir: compact1");
        ndp->ni_dent.d_reclen = spacefree + dsize;
    } else {
        if (spacefree < newentrysize)
            panic("wdir: compact2");
        ndp->ni_dent.d_reclen = spacefree;
        ep->d_reclen = dsize;
        ep = (struct direct *)((char *)ep + dsize);
    }
    bcopy((caddr_t)&ndp->ni_dent, (caddr_t)ep, (u_int)newentrysize);
    bwrite(bp);
    dp->i_flag |= IUPD|ICHG;
    if (ndp->ni_endoff && ndp->ni_endoff < dp->i_size)
        itrunc(dp, (u_long)ndp->ni_endoff, 0);
    iput(dp);
    return (error);
}

/*
 * Remove a directory entry after a call to namei, using the
 * parameters which it left in the u. area.  The u. entry
 * ni_offset contains the offset into the directory of the
 * entry to be eliminated.  The ni_count field contains the
 * size of the previous record in the directory.  If this
 * is 0, the first entry is being deleted, so we need only
 * zero the inode number to mark the entry as free.  If the
 * entry isn't the first in the directory, we must reclaim
 * the space of the now empty record by adding the record size
 * to the size of the previous entry.
 */
int
dirremove (struct nameidata *ndp)
{
    register struct inode *dp = ndp->ni_pdir;
    register struct buf *bp;
    struct direct *ep;

    if (ndp->ni_count == 0) {
        /*
         * First entry in block: set d_ino to zero.
         */
        ndp->ni_dent.d_ino = 0;
        (void) rdwri (UIO_WRITE, dp, (caddr_t) &ndp->ni_dent,
                (int) DIRSIZ(&ndp->ni_dent), ndp->ni_offset,
                IO_UNIT | IO_SYNC, (int*) 0);
    } else {
        /*
         * Collapse new free space into previous entry.
         */
        bp = blkatoff(dp, ndp->ni_offset - ndp->ni_count, (char **)&ep);
        if (bp == 0)
            return (0);
        ep->d_reclen += ndp->ni_dent.d_reclen;
        bwrite(bp);
        dp->i_flag |= IUPD|ICHG;
    }
    return (1);
}

/*
 * Rewrite an existing directory entry to point at the inode
 * supplied.  The parameters describing the directory entry are
 * set up by a call to namei.
 */
void
dirrewrite(struct inode *dp, struct inode *ip, struct nameidata *ndp)
{
    ndp->ni_dent.d_ino = ip->i_number;
    u.u_error = rdwri (UIO_WRITE, dp, (caddr_t) &ndp->ni_dent,
            (int) DIRSIZ(&ndp->ni_dent), ndp->ni_offset,
            IO_UNIT | IO_SYNC, (int*) 0);
    iput(dp);
}

/*
 * Check if a directory is empty or not.
 * Inode supplied must be locked.
 *
 * Using a struct dirtemplate here is not precisely
 * what we want, but better than using a struct direct.
 *
 * NB: does not handle corrupted directories.
 */
int
dirempty (struct inode *ip, ino_t parentino)
{
    register off_t off;
    struct dirtemplate dbuf;
    register struct direct *dp = (struct direct *)&dbuf;
    int error, count;
#define MINDIRSIZ (sizeof (struct dirtemplate) / 2)

    for (off = 0; off < ip->i_size; off += dp->d_reclen) {
        error = rdwri (UIO_READ, ip, (caddr_t) dp, MINDIRSIZ,
            off, IO_UNIT, &count);
        /*
         * Since we read MINDIRSIZ, residual must
         * be 0 unless we're at end of file.
         */
        if (error || count != 0)
            return (0);
        /* avoid infinite loops */
        if (dp->d_reclen == 0)
            return (0);
        /* skip empty entries */
        if (dp->d_ino == 0)
            continue;
        /* accept only "." and ".." */
        if (dp->d_namlen > 2)
            return (0);
        if (dp->d_name[0] != '.')
            return (0);
        /*
         * At this point d_namlen must be 1 or 2.
         * 1 implies ".", 2 implies ".." if second
         * char is also "."
         */
        if (dp->d_namlen == 1)
            continue;
        if (dp->d_name[1] == '.' && dp->d_ino == parentino)
            continue;
        return (0);
    }
    return (1);
}

/*
 * Check if source directory is in the path of the target directory.
 * Target is supplied locked, source is unlocked.
 * The target is always iput() before returning.
 */
int
checkpath (struct inode *source, struct inode *target)
{
    struct dirtemplate dirbuf;
    register struct inode *ip;
    register int error = 0;

    ip = target;
    if (ip->i_number == source->i_number) {
        error = EEXIST;
        goto out;
    }
    if (ip->i_number == ROOTINO)
        goto out;

    for (;;) {
        if ((ip->i_mode&IFMT) != IFDIR) {
            error = ENOTDIR;
            break;
        }
        error = rdwri (UIO_READ, ip, (caddr_t) &dirbuf,
                sizeof(struct dirtemplate), (off_t) 0,
                IO_UNIT, (int*) 0);
        if (error != 0)
            break;
        if (dirbuf.dotdot_namlen != 2 ||
            dirbuf.dotdot_name[0] != '.' ||
            dirbuf.dotdot_name[1] != '.') {
            error = ENOTDIR;
            break;
        }
        if (dirbuf.dotdot_ino == source->i_number) {
            error = EINVAL;
            break;
        }
        if (dirbuf.dotdot_ino == ROOTINO)
            break;
        iput(ip);
        ip = iget(ip->i_dev, ip->i_fs, dirbuf.dotdot_ino);
        if (ip == NULL) {
            error = u.u_error;
            break;
        }
    }

out:
    if (error == ENOTDIR)
        printf("checkpath: .. !dir\n");
    if (ip != NULL)
        iput(ip);
    return (error);
}

/*
 * Name cache initialization, from main() when we are booting
 */
void
nchinit()
{
    register union nchash *nchp;
    register struct namecache *ncp;

    nchhead = 0;
    nchtail = &nchhead;
    for (ncp = namecache; ncp < &namecache[NNAMECACHE]; ncp++) {
        ncp->nc_forw = ncp;         /* hash chain */
        ncp->nc_back = ncp;
        ncp->nc_nxt = NULL;         /* lru chain */
        *nchtail = ncp;
        ncp->nc_prev = nchtail;
        nchtail = &ncp->nc_nxt;
        /* all else is zero already */
    }
    for (nchp = nchash; nchp < &nchash[NCHHASH]; nchp++) {
        nchp->nch_head[0] = nchp;
        nchp->nch_head[1] = nchp;
    }
}

/*
 * Cache flush, called when filesys is umounted to
 * remove entries that would now be invalid
 *
 * The line "nxtcp = nchhead" near the end is to avoid potential problems
 * if the cache lru chain is modified while we are dumping the
 * inode.  This makes the algorithm O(n^2), but do you think I care?
 */
void
nchinval (dev_t dev)
{
    register struct namecache *ncp, *nxtcp;

    for (ncp = nchhead; ncp; ncp = nxtcp) {
        nxtcp = ncp->nc_nxt;
        if (ncp->nc_ip == NULL ||
            (ncp->nc_idev != dev && ncp->nc_dev != dev))
            continue;
        /* free the resources we had */
        ncp->nc_idev = NODEV;
        ncp->nc_dev = NODEV;
        ncp->nc_id = NULL;
        ncp->nc_ino = 0;
        ncp->nc_ip = NULL;
        remque(ncp);            /* remove entry from its hash chain */
        ncp->nc_forw = ncp;     /* and make a dummy one */
        ncp->nc_back = ncp;
        /* delete this entry from LRU chain */
        *ncp->nc_prev = nxtcp;
        if (nxtcp)
            nxtcp->nc_prev = ncp->nc_prev;
        else
            nchtail = ncp->nc_prev;
        /* cause rescan of list, it may have altered */
        nxtcp = nchhead;
        /* put the now-free entry at head of LRU */
        ncp->nc_nxt = nxtcp;
        ncp->nc_prev = &nchhead;
        nxtcp->nc_prev = &ncp->nc_nxt;
        nchhead = ncp;
    }
}

/*
 * Name cache invalidation of all entries.
 */
void
cinvalall()
{
    register struct namecache *ncp, *encp = &namecache[NNAMECACHE];

    for (ncp = namecache; ncp < encp; ncp++)
        ncp->nc_id = 0;
}
