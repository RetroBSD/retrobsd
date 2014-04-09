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

int
pass1bcheck(idesc)
    register struct inodesc *idesc;
{
    register daddr_t *dlp;
    daddr_t blkno = idesc->id_blkno;

    if (outrange(blkno))
        return (SKIP);
    for (dlp = duplist; dlp < muldup; dlp++) {
        if (*dlp == blkno) {
            blkerr(idesc->id_number, "DUP", blkno);
            *dlp = *--muldup;
            *muldup = blkno;
            return (muldup == duplist ? STOP : KEEPON);
        }
    }
    return (KEEPON);
}

void
pass1b()
{
    register DINODE *dp;
    struct inodesc idesc;
    ino_t inumber;

    bzero((char *)&idesc, sizeof(struct inodesc));
    idesc.id_type = ADDR;
    idesc.id_func = pass1bcheck;
    for (inumber = ROOTINO; inumber < lastino; inumber++) {
        if (inumber < ROOTINO)
            continue;
        dp = ginode(inumber);
        if (dp == NULL)
            continue;
        idesc.id_number = inumber;
        if (getstate(inumber) != USTATE &&
            (ckinode(dp, &idesc) & STOP))
            goto out1b;
    }
out1b:
    flush(&dfile, &inoblk);
}
