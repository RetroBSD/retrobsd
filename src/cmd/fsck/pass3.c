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
pass3()
{
    register DINODE *dp;
    struct inodesc idesc;
    ino_t inumber, orphan;

    bzero((char *)&idesc, sizeof(struct inodesc));
    idesc.id_type = DATA;
    for (inumber = ROOTINO; inumber <= lastino; inumber++) {
        if (getstate(inumber) == DSTATE) {
            pathp = pathname;
            *pathp++ = '?';
            *pathp = '\0';
            idesc.id_func = findino;
            idesc.id_name = "..";
            idesc.id_parent = inumber;
            do {
                orphan = idesc.id_parent;
                if (orphan < ROOTINO || orphan > imax)
                    break;
                dp = ginode(orphan);
                idesc.id_parent = 0;
                idesc.id_number = orphan;
                (void)ckinode(dp, &idesc);
                if (idesc.id_parent == 0)
                    break;
            } while (getstate(idesc.id_parent) == DSTATE);
            if (linkup(orphan, idesc.id_parent) == 1) {
                idesc.id_func = pass2check;
                idesc.id_number = lfdir;
                descend(&idesc, orphan);
            }
        }
    }
}
