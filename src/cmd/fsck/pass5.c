/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdio.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include "fsck.h"

void
ifreechk()
{
    register int i;
    ino_t   inum;

    for (i=0; i<sblock.fs_ninode; i++) {
        inum = sblock.fs_inode[i];
        switch (getstate(inum)) {

        case USTATE:
            continue;
        default:
            pwarn("ALLOCATED INODE(S) IN IFREE LIST");
            if (preen)
                printf(" (FIXED)\n");
            if (preen || reply("FIX") == 1) {
                sblock.fs_ninode = i-1;
                sbdirty();
            }
            return;
        }
    }
}

int
pass5check(blk)
    daddr_t blk;
{
    if (outrange(blk)) {
        fixfree = 1;
        if (preen)
            pfatal("BAD BLOCKS IN FREE LIST.\n");
        if (++badblk >= MAXBAD) {
            printf("EXCESSIVE BAD BLKS IN FREE LIST.");
            if (reply("CONTINUE") == 0)
                errexit("");
            return(STOP);
        }
        return(SKIP);
    }
    if (getfmap(blk)) {
        fixfree = 1;
        if (++dupblk >= DUPTBLSIZE) {
            printf("EXCESSIVE DUP BLKS IN FREE LIST.");
            if (reply("CONTINUE") == 0)
                errexit("");
            return(STOP);
        }
    } else {
        n_free++;
        setfmap(blk);
    }
    return(KEEPON);
}

void
freechk()
{
    register daddr_t *ap;

    if (freeblk.df_nfree == 0)
        return;
    do {
        if (freeblk.df_nfree <= 0 || freeblk.df_nfree > NICFREE) {
            pfatal("BAD FREEBLK COUNT\n");
            fixfree = 1;
            return;
        }
        ap = &freeblk.df_free[freeblk.df_nfree];
        while (--ap > &freeblk.df_free[0]) {
            if (pass5check(*ap) == STOP)
                return;
        }
        if (*ap == (daddr_t)0 || pass5check(*ap) != KEEPON)
            return;
    } while (getblk(&fileblk,*ap) != NULL);
}

void
makefree()
{
    register int i;
    daddr_t blk;

    sblock.fs_nfree = 0;
    sblock.fs_flock = 0;
    sblock.fs_fmod = 0;
    sblock.fs_tfree = 0;
    sblock.fs_ninode = 0;
    sblock.fs_ilock = 0;
    sblock.fs_ronly = 0;
    bzero((char *)&freeblk, DEV_BSIZE);
    freeblk.df_nfree++;
    for (blk = fsmax-1; ! outrange(blk); blk--) {
        if (! getbmap(blk)) {
            sblock.fs_tfree++;
            if (freeblk.df_nfree >= NICFREE) {
                fbdirty();
                fileblk.b_bno = blk;
                flush(&dfile, &fileblk);
                bzero((char *)&freeblk, DEV_BSIZE);
            }
            freeblk.df_free[freeblk.df_nfree] = blk;
            freeblk.df_nfree++;
        }
    }
    sblock.fs_nfree = freeblk.df_nfree;
    for (i = 0; i < NICFREE; i++)
        sblock.fs_free[i] = freeblk.df_free[i];
    sbdirty();
}

void
pass5()
{
    struct inodesc idesc;
    daddr_t blkno;
    int n;
    BUFAREA *bp1, *bp2;
    extern int debug;

    bzero(&idesc, sizeof (idesc));
    idesc.id_type = ADDR;

    if (sflag)
        fixfree = 1;
    else {
        ifreechk();
        if (freemap)
            bcopy(blockmap, freemap, (int)bmapsz);
        else {
            for (blkno = 0; blkno < fmapblk-bmapblk; blkno++) {
                bp1 = getblk((BUFAREA *)NULL,blkno+bmapblk);
                bp2 = getblk((BUFAREA *)NULL,blkno+fmapblk);
                bcopy(bp1->b_un.b_buf,bp2->b_un.b_buf,DEV_BSIZE);
                dirty(bp2);
            }
        }
        badblk = dupblk = 0;
        freeblk.df_nfree = sblock.fs_nfree;
        for (n = 0; n < NICFREE; n++)
            freeblk.df_free[n] = sblock.fs_free[n];
        freechk();
        if (badblk)
            pfatal("%d BAD BLKS IN FREE LIST\n",badblk);
        if (dupblk)
            pwarn("%d DUP BLKS IN FREE LIST\n",dupblk);
        if (debug)
            printf("n_files %ld n_blks %ld n_free %ld fsmax %ld fsmin %ld ninode: %u\n",
                n_files, n_blks, n_free, fsmax, fsmin, sblock.fs_ninode);
        if (fixfree == 0) {
            if ((n_blks+n_free) != (fsmax-fsmin) &&
                    dofix(&idesc, "BLK(S) MISSING"))
                fixfree = 1;
            else if (n_free != sblock.fs_tfree &&
               dofix(&idesc,"FREE BLK COUNT WRONG IN SUPERBLOCK")) {
                    sblock.fs_tfree = n_free;
                    sbdirty();
                }
        }
        if (fixfree && !dofix(&idesc, "BAD FREE LIST"))
                fixfree = 0;
    }

    if (fixfree) {
        if (preen == 0)
            printf("** Phase 6 - Salvage Free List\n");
        makefree();
        n_free = sblock.fs_tfree;
        sblock.fs_ronly = 0;
        sblock.fs_fmod = 0;
        sbdirty();
    }
}
