/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <pwd.h>
#include <time.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include "fsck.h"

int
iblock(idesc, ilevel, isize)
    struct inodesc *idesc;
    register int ilevel;
    long isize;
{
    register daddr_t *ap;
    register daddr_t *aplim;
    int i, n, (*func)(), nif;
    long sizepb;
    BUFAREA ib;
    char buf[128];

    if (idesc->id_type == ADDR) {
        func = idesc->id_func;
        if (((n = (*func)(idesc)) & KEEPON) == 0)
            return (n);
    } else
        func = dirscan;
    if (outrange(idesc->id_blkno)) /* protect thyself */
        return (SKIP);
    initbarea(&ib);
    getblk(&ib, idesc->id_blkno);
    if (ib.b_errs != NULL)
        return (SKIP);
    ilevel--;
    for (sizepb = DEV_BSIZE, i = 0; i < ilevel; i++)
        sizepb *= NINDIR;
    nif = isize / sizepb + 1;
    if (nif > NINDIR)
        nif = NINDIR;
    if (idesc->id_func == pass1check && nif < NINDIR) {
        aplim = &ib.b_un.b_indir[NINDIR];
        for (ap = &ib.b_un.b_indir[nif]; ap < aplim; ap++) {
            if (*ap == 0)
                continue;
            sprintf(buf, "PARTIALLY TRUNCATED INODE I=%u",
                idesc->id_number);
            if (dofix(idesc, buf)) {
                *ap = 0;
                dirty(&ib);
            }
        }
        flush(&dfile, &ib);
    }
    aplim = &ib.b_un.b_indir[nif];
    for (ap = ib.b_un.b_indir, i = 1; ap < aplim; ap++, i++)
        if (*ap) {
            idesc->id_blkno = *ap;
            if (ilevel > 0)
                n = iblock(idesc, ilevel, isize - i * sizepb);
            else
                n = (*func)(idesc);
            if (n & STOP)
                return (n);
        }
    return (KEEPON);
}

int
ckinode(dp, idesc)
    DINODE *dp;
    register struct inodesc *idesc;
{
    register daddr_t *ap;
    int ret, n;
    DINODE dino;

    idesc->id_fix = DONTKNOW;
    idesc->id_entryno = 0;
    idesc->id_filesize = dp->di_size;
    if (SPECIAL(dp))
        return (KEEPON);
    dino = *dp;
    for (ap = &dino.di_addr[0]; ap < &dino.di_addr[NDADDR]; ap++) {
        if (*ap == 0)
            continue;
        idesc->id_blkno = *ap;
        if (idesc->id_type == ADDR) {
            ret = (*idesc->id_func)(idesc);
        } else {
            ret = dirscan(idesc);
        }
        if (ret & STOP)
            return (ret);
    }
    for (ap = &dino.di_addr[NDADDR], n = 1; n <= NIADDR; ap++, n++) {
        if (*ap) {
            idesc->id_blkno = *ap;
            ret = iblock(idesc, n,
                dino.di_size - DEV_BSIZE * NDADDR);
            if (ret & STOP)
                return (ret);
        }
    }
    return (KEEPON);
}

DINODE *
ginode(inumber)
    register ino_t inumber;
{
    daddr_t iblk;
    register DINODE *dp;

    if (inumber < ROOTINO || inumber > imax)
        errexit("bad inode number %u to ginode\n", inumber);
    iblk = itod(inumber);
    if (iblk < startib || iblk >= startib + NINOBLK) {
        if (inoblk.b_dirty) {
            bwrite(&dfile, inobuf, startib, NINOBLK * DEV_BSIZE);
            inoblk.b_dirty = 0;
        }
        if (bread(&dfile, inobuf, iblk, NINOBLK * DEV_BSIZE))
            return(NULL);
        startib = iblk;
    }
    dp = (DINODE *) &inobuf[(unsigned)((iblk-startib)<<DEV_BSHIFT)];
    return (dp + itoo(inumber));
}

void
clri(idesc, s, flg)
    register struct inodesc *idesc;
    char *s;
    int flg;
{
    register DINODE *dp;

    dp = ginode(idesc->id_number);
    if (flg == 1) {
        pwarn("%s %s", s, DIRCT(dp) ? "DIR" : "FILE");
        pinode(idesc->id_number);
    }
    if (preen || reply("CLEAR") == 1) {
        if (preen)
            printf(" (CLEARED)\n");
        n_files--;
        (void)ckinode(dp, idesc);
        zapino(dp);
        setstate(idesc->id_number, USTATE);
        inodirty();
    }
}

int
findname(idesc)
    struct inodesc *idesc;
{
    register DIRECT *dirp = idesc->id_dirp;

    if (dirp->d_ino != idesc->id_parent)
        return (KEEPON);
    bcopy(dirp->d_name, idesc->id_name, dirp->d_namlen + 1);
    return (STOP);
}

int
findino(idesc)
    struct inodesc *idesc;
{
    register DIRECT *dirp = idesc->id_dirp;

    if (dirp->d_ino == 0)
        return (KEEPON);
    if (strcmp(dirp->d_name, idesc->id_name) == 0 &&
        dirp->d_ino >= ROOTINO && dirp->d_ino <= imax) {
        idesc->id_parent = dirp->d_ino;
        return (STOP);
    }
    return (KEEPON);
}

void
pinode(ino)
    ino_t ino;
{
    register DINODE *dp;
    register char *p;
    struct passwd *pw;

    printf(" I=%u ", ino);
    if (ino < ROOTINO || ino > imax)
        return;
    dp = ginode(ino);
    printf(" OWNER=");
    if ((pw = getpwuid((int)dp->di_uid)) != 0)
        printf("%s ", pw->pw_name);
    else
        printf("%u ", dp->di_uid);
    printf("MODE=%o\n", dp->di_mode);
    if (preen)
        printf("%s: ", devnam);
    printf("SIZE=%ld ", dp->di_size);
    p = ctime(&dp->di_mtime);
    printf("MTIME=%12.12s %4.4s ", p+4, p+20);
}

void
blkerr(ino, s, blk)
    ino_t ino;
    char *s;
    daddr_t blk;
{

    pfatal("%ld %s I=%u\n", blk, s, ino);
    printf("\n");
    switch (getstate(ino)) {

    case FSTATE:
        setstate(ino, FCLEAR);
        return;

    case DSTATE:
        setstate(ino, DCLEAR);
        return;

    case FCLEAR:
    case DCLEAR:
        return;

    default:
        errexit("BAD STATE %d TO BLKERR\n", getstate(ino));
        /* NOTREACHED */
    }
}

/*
 * allocate an unused inode
 */
ino_t
allocino(request, type)
    ino_t request;
    int type;
{
    register ino_t ino;
    register DINODE *dp;

    if (request == 0)
        request = ROOTINO;
    else if (getstate(request) != USTATE)
        return (0);
    for (ino = request; ino < imax; ino++)
        if (getstate(ino) == USTATE)
            break;
    if (ino == imax)
        return (0);
    switch (type & IFMT) {
    case IFDIR:
        setstate(ino, DSTATE);
        break;
    case IFREG:
    case IFLNK:
        setstate(ino, FSTATE);
        break;
    default:
        return (0);
    }
    dp = ginode(ino);
    dp->di_addr[0] = allocblk();
    if (dp->di_addr[0] == 0) {
        setstate(ino, USTATE);
        return (0);
    }
    dp->di_mode = type;
    time(&dp->di_atime);
    dp->di_mtime = dp->di_ctime = dp->di_atime;
    dp->di_size = DEV_BSIZE;
    n_files++;
    inodirty();
    return (ino);
}

/*
 * deallocate an inode
 */
void
freeino(ino)
    ino_t ino;
{
    struct inodesc idesc;
    DINODE *dp;

    bzero((char *)&idesc, sizeof(struct inodesc));
    idesc.id_type = ADDR;
    idesc.id_func = pass4check;
    idesc.id_number = ino;
    dp = ginode(ino);
    (void)ckinode(dp, &idesc);
    zapino(dp);
    inodirty();
    setstate(ino, USTATE);
    n_files--;
}
