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
pass1()
{
    register int j;
    register DINODE *dp;
    daddr_t ndb, lj;
    struct inodesc idesc;
    register ino_t inumber;

    /*
     * Set file system reserved blocks in used block map.
     */
    for (j = 0; j < fsmin; j++)
        setbmap((daddr_t)j);
    /*
     * Find all allocated blocks.
     */
    bzero((char *)&idesc, sizeof(struct inodesc));
    idesc.id_type = ADDR;
    idesc.id_func = pass1check;
    n_files = n_blks = n_free = 0;
    for (inumber = ROOTINO; inumber < imax; inumber++) {
        dp = ginode(inumber);
        if (!ALLOC(dp)) {
            if (bcmp((char *)dp->di_addr, (char *)zino.di_addr,
                NADDR * sizeof(daddr_t)) ||
                dp->di_mode || dp->di_size) {
                pfatal("PARTIALLY ALLOCATED INODE I=%u\n",
                    inumber);
                if (reply("CLEAR") == 1) {
                    zapino(dp);
                    inodirty();
                }
            }
            setstate(inumber, USTATE);
            continue;
        }
        lastino = inumber;
        if (dp->di_size < 0) {
            printf("bad size %ld:", dp->di_size);
            goto unknown;
        }
        if (!preen && (dp->di_mode & IFMT) == IFMT &&
            reply("HOLD BAD BLOCK") == 1) {
            dp->di_size = sblock.fs_fsize;
            dp->di_mode = IFREG|0600;
            inodirty();
        }
        ndb = howmany(dp->di_size, DEV_BSIZE);
        if (SPECIAL(dp))
            ndb++;
/*
 * This check is not in 4.3BSD and is due to the fact that pipes in 2.11BSD
 * are still implemented using the filesystem.  Zero length files with blocks
 * (typically only the first direct block) allocated are the symptom.  It is
 * safe to clear the inode as the blocks will end up missing and be reclaimed
 * in pass5.
 */
        else if (dp->di_size == 0 && bcmp(dp->di_addr,
                zino.di_addr,NADDR* sizeof (daddr_t))) {
            pwarn("SIZE=0 FILE HAS ALLOCATED BLOCKS. I=%u",inumber);
            if (preen)
                printf(" (CLEARED)\n");
            if (preen || reply("CLEAR") == 1) {
                setstate(inumber, USTATE);
                zapino(dp);
                inodirty();
                continue;
            }
        }
        for (lj = ndb; lj < NDADDR; lj++) {
            if (lj == 1 && SPECIAL(dp))
                continue;
            j = lj;
            if (dp->di_addr[j] != 0) {
                if (debug)
                    printf("bad direct di_addr[%d]: %ld\n",
                        j, dp->di_addr[j]);
                goto unknown;
            }
        }
        for (j = 0, ndb -= NDADDR; ndb > 0; j++)
            ndb /= NINDIR;
        for (; j < NIADDR; j++)
            if (dp->di_addr[NDADDR + j] != 0) {
                if (debug)
                    printf("bad indirect addr: %ld\n",
                        dp->di_addr[NDADDR + j]);
                goto unknown;
            }
        if (ftypeok(dp) == 0)
            goto unknown;
        n_files++;
        setlncnt(inumber, dp->di_nlink);
        if (dp->di_nlink <= 0) {
            if (zlnp >= &zlnlist[MAXLNCNT]) {
                pfatal("LINK COUNT TABLE OVERFLOW\n");
                if (reply("CONTINUE") == 0)
                    errexit("");
            } else
                *zlnp++ = inumber;
        }
        setstate(inumber, DIRCT(dp) ? DSTATE : FSTATE);
        badblk = dupblk = 0;
        idesc.id_number = inumber;
        (void)ckinode(dp, &idesc);
        continue;
unknown:
        pfatal("UNKNOWN FILE TYPE I=%u mode: %o\n", inumber, dp->di_mode);
        setstate(inumber, FCLEAR);
        if (reply("CLEAR") == 1) {
            setstate(inumber, USTATE);
            zapino(dp);
            inodirty();
        }
    }
}

int
pass1check(idesc)
    register struct inodesc *idesc;
{
    int res = KEEPON;
    daddr_t blkno = idesc->id_blkno;
    register daddr_t *dlp;

    if (outrange(blkno)) {
        blkerr(idesc->id_number, "BAD", blkno);
        if (++badblk >= MAXBAD) {
            pwarn("EXCESSIVE BAD BLKS I=%u", idesc->id_number);
            if (preen)
                printf(" (SKIPPING)\n");
            else if (reply("CONTINUE") == 0)
                errexit("");
            return (STOP);
        }
        return (SKIP);
    }
    if (!getbmap(blkno)) {
            n_blks++;
            setbmap(blkno);
    } else {
        blkerr(idesc->id_number, "DUP", blkno);
        if (++dupblk >= MAXDUP) {
            pwarn("EXCESSIVE DUP BLKS I=%u",
                idesc->id_number);
            if (preen)
                printf(" (SKIPPING)\n");
            else if (reply("CONTINUE") == 0)
                errexit("");
            return (STOP);
        }
        if (enddup >= &duplist[DUPTBLSIZE]) {
            pfatal("DUP TABLE OVERFLOW\n");
            if (reply("CONTINUE") == 0)
                errexit("");
            return(STOP);
        }
        for (dlp = duplist; dlp < muldup; dlp++) {
            if (*dlp == blkno) {
                *enddup++ = blkno;
                break;
            }
        }
        if ( dlp >= muldup) {
            *enddup++ = *muldup;
            *muldup++ = blkno;
        }
    }
    /*
     * count the number of blocks found in id_entryno
     */
    idesc->id_entryno++;
    return (res);
}
