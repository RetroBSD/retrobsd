/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include "fsck.h"

#define MINDIRSIZE  (sizeof (struct dirtemplate))

char    *endpathname = &pathname[MAXPATHLEN - 2];
char    *lfname = "lost+found";
struct  dirtemplate emptydir = { 0, DIRBLKSIZ };
/*
 * The strange initialization is due to quoted strings causing problems when
 * 'xstr' is used to preprocess the sources.  The structure has two members
 * as 'char dot_name[2]' and 'char dotdot_name[6]' which is NOT the same as
 * 'char *'.
 */
struct  dirtemplate dirhead = { 0, 8, 1, {'.'}, 0, DIRBLKSIZ - 8, 2,
                {'.', '.' }};

void
descend(parentino, inumber)
    struct inodesc *parentino;
    ino_t inumber;
{
    register DINODE *dp;
    struct inodesc curino;

    bzero((char *)&curino, sizeof(struct inodesc));
    if (getstate(inumber) != DSTATE)
        errexit("BAD INODE %u TO DESCEND\n", getstate(inumber));
    setstate(inumber, DFOUND);
    dp = ginode(inumber);
    if (dp->di_size == 0) {
        direrr(inumber, "ZERO LENGTH DIRECTORY");
        if (reply("REMOVE") == 1)
            setstate(inumber, DCLEAR);
        return;
    }
    if (dp->di_size < MINDIRSIZE) {
        direrr(inumber, "DIRECTORY TOO SHORT");
        dp->di_size = MINDIRSIZE;
        if (reply("FIX") == 1)
            inodirty();
    }
    if ((dp->di_size & (DIRBLKSIZ - 1)) != 0) {
        pwarn("DIRECTORY %s: LENGTH %ld NOT MULTIPLE OF %d",
            pathname, dp->di_size, DIRBLKSIZ);
        dp->di_size = roundup(dp->di_size, DIRBLKSIZ);
        if (preen)
            printf(" (ADJUSTED)\n");
        if (preen || reply("ADJUST") == 1)
            inodirty();
    }
    curino.id_type = DATA;
    curino.id_func = parentino->id_func;
    curino.id_parent = parentino->id_number;
    curino.id_number = inumber;
    (void)ckinode(dp, &curino);
}

/*
 * Verify that a directory entry is valid.
 * This is a superset of the checks made in the kernel.
 */
int
dircheck(idesc, dp)
    struct inodesc *idesc;
    register DIRECT *dp;
{
    register int size;
    register char *cp;
    int spaceleft;

    size = DIRSIZ(dp);
    spaceleft = DIRBLKSIZ - (idesc->id_loc % DIRBLKSIZ);
    if (dp->d_ino < imax &&
        dp->d_reclen != 0 &&
        dp->d_reclen <= spaceleft &&
        (dp->d_reclen & 0x3) == 0 &&
        dp->d_reclen >= size &&
        idesc->id_filesize >= size &&
        dp->d_namlen <= MAXNAMLEN) {
        if (dp->d_ino == 0)
            return (1);
        for (cp = dp->d_name, size = 0; size < dp->d_namlen; size++)
            if (*cp == 0 || (*cp++ & 0200))
                return (0);
        if (*cp == 0)
            return (1);
    }
    return (0);
}

/*
 * get next entry in a directory.
 */
DIRECT *
fsck_readdir(idesc)
    register struct inodesc *idesc;
{
    register DIRECT *dp, *ndp;
    u_int size;

    getblk(&fileblk, idesc->id_blkno);
    if (fileblk.b_errs != NULL) {
        idesc->id_filesize -= DEV_BSIZE - idesc->id_loc;
        return NULL;
    }
    if (idesc->id_loc % DIRBLKSIZ == 0 && idesc->id_filesize > 0 &&
        idesc->id_loc < DEV_BSIZE) {
        dp = (DIRECT *)(dirblk.b_buf + idesc->id_loc);
        if (dircheck(idesc, dp))
            goto dpok;
        idesc->id_loc += DIRBLKSIZ;
        idesc->id_filesize -= DIRBLKSIZ;
        dp->d_reclen = DIRBLKSIZ;
        dp->d_ino = 0;
        dp->d_namlen = 0;
        dp->d_name[0] = '\0';
        if (dofix(idesc, "DIRECTORY CORRUPTED"))
            dirty(&fileblk);
        return (dp);
    }
dpok:
    if (idesc->id_filesize <= 0 || idesc->id_loc >= DEV_BSIZE)
        return NULL;
    dp = (DIRECT *)(dirblk.b_buf + idesc->id_loc);
    idesc->id_loc += dp->d_reclen;
    idesc->id_filesize -= dp->d_reclen;
    if ((idesc->id_loc % DIRBLKSIZ) == 0)
        return (dp);
    ndp = (DIRECT *)(dirblk.b_buf + idesc->id_loc);
    if (idesc->id_loc < DEV_BSIZE && idesc->id_filesize > 0 &&
        dircheck(idesc, ndp) == 0) {
        size = DIRBLKSIZ - (idesc->id_loc % DIRBLKSIZ);
        dp->d_reclen += size;
        idesc->id_loc += size;
        idesc->id_filesize -= size;
        if (dofix(idesc, "DIRECTORY CORRUPTED"))
            dirty(&fileblk);
    }
    return (dp);
}

int
dirscan(idesc)
    register struct inodesc *idesc;
{
    register DIRECT *dp;
    int dsize, n;
    char dbuf[DIRBLKSIZ];

    if (idesc->id_type != DATA)
        errexit("wrong type to dirscan %d\n", idesc->id_type);
    if (idesc->id_entryno == 0 &&
        (idesc->id_filesize & (DIRBLKSIZ - 1)) != 0)
        idesc->id_filesize = roundup(idesc->id_filesize, DIRBLKSIZ);
    if (outrange(idesc->id_blkno)) {
        idesc->id_filesize -= DEV_BSIZE;
        return (SKIP);
    }
    idesc->id_loc = 0;
    for (dp = fsck_readdir(idesc); dp != NULL; dp = fsck_readdir(idesc)) {
        dsize = dp->d_reclen;
        bcopy((char *)dp, dbuf, dsize);
        idesc->id_dirp = (DIRECT *)dbuf;
        if ((n = (*idesc->id_func)(idesc)) & ALTERED) {
            getblk(&fileblk, idesc->id_blkno);
            if (fileblk.b_errs != NULL) {
                n &= ~ALTERED;
            } else {
                bcopy(dbuf, (char *)dp, dsize);
                dirty(&fileblk);
                sbdirty();
            }
        }
        if (n & STOP)
            return (n);
    }
    return (idesc->id_filesize > 0 ? KEEPON : STOP);
}

void
direrr(ino, s)
    ino_t ino;
    char *s;
{
    register DINODE *dp;

    pwarn("%s ", s);
    pinode(ino);
    printf("\n");
    if (ino < ROOTINO || ino > imax) {
        pfatal("NAME=%s\n", pathname);
        return;
    }
    dp = ginode(ino);
    if (ftypeok(dp))
        pfatal("%s=%s\n", DIRCT(dp) ? "DIR" : "FILE", pathname);
    else
        pfatal("NAME=%s\n", pathname);
}

void
adjust(idesc, lcnt)
    register struct inodesc *idesc;
    short lcnt;
{
    register DINODE *dp;

    dp = ginode(idesc->id_number);
    if (dp->di_nlink == lcnt) {
        if (linkup(idesc->id_number, (ino_t)0) == 0)
            clri(idesc, "UNREF", 0);
    } else {
        pwarn("LINK COUNT %s", (lfdir == idesc->id_number) ? lfname :
            (DIRCT(dp) ? "DIR" : "FILE"));
        pinode(idesc->id_number);
        printf(" COUNT %u SHOULD BE %u",
            dp->di_nlink, dp->di_nlink-lcnt);
        if (preen) {
            if (lcnt < 0) {
                printf("\n");
                pfatal("LINK COUNT INCREASING\n");
            }
            printf(" (ADJUSTED)\n");
        }
        if (preen || reply("ADJUST") == 1) {
            dp->di_nlink -= lcnt;
            inodirty();
        }
    }
}

int
mkentry(idesc)
    struct inodesc *idesc;
{
    register DIRECT *dirp = idesc->id_dirp;
    DIRECT newent;
    int newlen, oldlen;

    newent.d_namlen = 11;
    newlen = DIRSIZ(&newent);
    if (dirp->d_ino != 0)
        oldlen = DIRSIZ(dirp);
    else
        oldlen = 0;
    if (dirp->d_reclen - oldlen < newlen)
        return (KEEPON);
    newent.d_reclen = dirp->d_reclen - oldlen;
    dirp->d_reclen = oldlen;
    dirp = (struct direct *)(((char *)dirp) + oldlen);
    dirp->d_ino = idesc->id_parent; /* ino to be entered is in id_parent */
    dirp->d_reclen = newent.d_reclen;
    dirp->d_namlen = strlen(idesc->id_name);
    bcopy(idesc->id_name, dirp->d_name, dirp->d_namlen + 1);
    return (ALTERED|STOP);
}

int
chgino(idesc)
    struct inodesc *idesc;
{
    register DIRECT *dirp = idesc->id_dirp;

    if (bcmp(dirp->d_name, idesc->id_name, dirp->d_namlen + 1))
        return (KEEPON);
    dirp->d_ino = idesc->id_parent;;
    return (ALTERED|STOP);
}

/*
 * Attempt to expand the size of a directory
 */
int
expanddir(dp)
    register DINODE *dp;
{
    daddr_t lastbn, newblk;
    char *cp, firstblk[DIRBLKSIZ];
    struct inodesc idesc;

    lastbn = lblkno(dp->di_size);
    if (lastbn >= NDADDR - 1)
        return (0);
    if ((newblk = allocblk()) == 0)
        return (0);
    dp->di_addr[lastbn + 1] = dp->di_addr[lastbn];
    dp->di_addr[lastbn] = newblk;
    dp->di_size += DEV_BSIZE;
    getblk(&fileblk, dp->di_addr[lastbn + 1]);
    if (fileblk.b_errs != NULL)
        goto bad;
    bcopy(dirblk.b_buf, firstblk, DIRBLKSIZ);
    getblk(&fileblk, newblk);
    if (fileblk.b_errs != NULL)
        goto bad;
    bcopy(firstblk, dirblk.b_buf, DIRBLKSIZ);
    for (cp = &dirblk.b_buf[DIRBLKSIZ];
         cp < &dirblk.b_buf[DEV_BSIZE];
         cp += DIRBLKSIZ)
        bcopy((char *)&emptydir, cp, sizeof emptydir);
    dirty(&fileblk);
    getblk(&fileblk, dp->di_addr[lastbn + 1]);
    if (fileblk.b_errs != NULL)
        goto bad;
    bcopy((char *)&emptydir, dirblk.b_buf, sizeof emptydir);
    pwarn("NO SPACE LEFT IN %s", pathname);
    if (preen)
        printf(" (EXPANDED)\n");
    else if (reply("EXPAND") == 0)
        goto bad;
    dirty(&fileblk);
    inodirty();
    return (1);
bad:
    dp->di_addr[lastbn] = dp->di_addr[lastbn + 1];
    dp->di_addr[lastbn + 1] = 0;
    dp->di_size -= DEV_BSIZE;

    /* Free a previously allocated block */
    idesc.id_blkno = newblk;
    pass4check(&idesc);
    return (0);
}

/*
 * make an entry in a directory
 */
int
makeentry(parent, ino, name)
    ino_t parent, ino;
    char *name;
{
    DINODE *dp;
    struct inodesc idesc;

    if (parent < ROOTINO || parent >= imax || ino < ROOTINO || ino >= imax)
        return (0);
    bzero(&idesc, sizeof(struct inodesc));
    idesc.id_type = DATA;
    idesc.id_func = mkentry;
    idesc.id_number = parent;
    idesc.id_parent = ino;  /* this is the inode to enter */
    idesc.id_fix = DONTKNOW;
    idesc.id_name = name;
    dp = ginode(parent);
    if (dp->di_size % DIRBLKSIZ) {
        dp->di_size = roundup(dp->di_size, DIRBLKSIZ);
        inodirty();
    }
    if ((ckinode(dp, &idesc) & ALTERED) != 0)
        return (1);
    if (expanddir(dp) == 0)
        return (0);
    return (ckinode(dp, &idesc) & ALTERED);
}

/*
 * free a directory inode
 */
void
freedir(ino, parent)
    ino_t ino, parent;
{
    DINODE *dp;

    if (ino != parent) {
        dp = ginode(parent);
        dp->di_nlink--;
        inodirty();
    }
    freeino(ino);
}

/*
 * generate a temporary name for the lost+found directory.
 */
int
lftempname(bufp, ino)
    char *bufp;
    ino_t ino;
{
    register ino_t in;
    register char *cp;
    int namlen;

    cp = bufp + 2;
    for (in = imax; in > 0; in /= 10)
        cp++;
    *--cp = 0;
    namlen = cp - bufp;
    in = ino;
    while (cp > bufp) {
        *--cp = (in % 10) + '0';
        in /= 10;
    }
    *cp = '#';
    return (namlen);
}

int
linkup(orphan, pdir)
    ino_t orphan;
    ino_t pdir;
{
    register DINODE *dp;
    int lostdir, len;
    ino_t oldlfdir;
    struct inodesc idesc;
    char tempname[MAXPATHLEN];

    bzero((char *)&idesc, sizeof(struct inodesc));
    dp = ginode(orphan);
    lostdir = DIRCT(dp);
    pwarn("UNREF %s ", lostdir ? "DIR" : "FILE");
    pinode(orphan);
    if (preen && dp->di_size == 0)
        return (0);
    if (preen)
        printf(" (RECONNECTED)\n");
    else
        if (reply("RECONNECT") == 0)
            return (0);
    pathp = pathname;
    *pathp++ = '/';
    *pathp = '\0';
    if (lfdir == 0) {
        dp = ginode(ROOTINO);
        idesc.id_name = lfname;
        idesc.id_type = DATA;
        idesc.id_func = findino;
        idesc.id_number = ROOTINO;
        (void)ckinode(dp, &idesc);
        if (idesc.id_parent >= ROOTINO && idesc.id_parent < imax) {
            lfdir = idesc.id_parent;
        } else {
            pwarn("NO lost+found DIRECTORY");
            if (preen || reply("CREATE")) {
                lfdir = allocdir(ROOTINO, 0);
                if (lfdir != 0) {
                    if (makeentry(ROOTINO, lfdir, lfname) != 0) {
                        if (preen)
                            printf(" (CREATED)\n");
                    } else {
                        freedir(lfdir, ROOTINO);
                        lfdir = 0;
                        if (preen)
                            printf("\n");
                    }
                }
            }
        }
        if (lfdir == 0) {
            pfatal("SORRY. CANNOT CREATE lost+found DIRECTORY\n\n");
            return (0);
        }
    }
    dp = ginode(lfdir);
    if (!DIRCT(dp)) {
        pfatal("lost+found IS NOT A DIRECTORY\n");
        if (reply("REALLOCATE") == 0)
            return (0);
        oldlfdir = lfdir;
        if ((lfdir = allocdir(ROOTINO, 0)) == 0) {
            pfatal("SORRY. CANNOT CREATE lost+found DIRECTORY\n\n");
            return (0);
        }
        idesc.id_type = DATA;
        idesc.id_func = chgino;
        idesc.id_number = ROOTINO;
        idesc.id_parent = lfdir;    /* new inumber for lost+found */
        idesc.id_name = lfname;
        if ((ckinode(ginode(ROOTINO), &idesc) & ALTERED) == 0) {
            pfatal("SORRY. CANNOT CREATE lost+found DIRECTORY\n\n");
            return (0);
        }
        inodirty();
        idesc.id_type = ADDR;
        idesc.id_func = pass4check;
        idesc.id_number = oldlfdir;
        adjust(&idesc, getlncnt(oldlfdir) + 1);
        setlncnt(oldlfdir, 0);
        dp = ginode(lfdir);
    }
    if (getstate(lfdir) != DFOUND) {
        pfatal("SORRY. NO lost+found DIRECTORY\n\n");
        return (0);
    }
    len = strlen(lfname);
    bcopy(lfname, pathp, len + 1);
    pathp += len;
    len = lftempname(tempname, orphan);
    if (makeentry(lfdir, orphan, tempname) == 0) {
        pfatal("SORRY. NO SPACE IN lost+found DIRECTORY\n");
        printf("\n\n");
        return (0);
    }
    declncnt(orphan);
    *pathp++ = '/';
    bcopy(idesc.id_name, pathp, len + 1);
    pathp += len;
    if (lostdir) {
        dp = ginode(orphan);
        idesc.id_type = DATA;
        idesc.id_func = chgino;
        idesc.id_number = orphan;
        idesc.id_fix = DONTKNOW;
        idesc.id_name = "..";
        idesc.id_parent = lfdir;    /* new value for ".." */
        (void)ckinode(dp, &idesc);
        dp = ginode(lfdir);
        dp->di_nlink++;
        inodirty();
        inclncnt(lfdir);
        pwarn("DIR I=%u CONNECTED. ", orphan);
        printf("PARENT WAS I=%u\n", pdir);
        if (preen == 0)
            printf("\n");
    }
    return (1);
}

/*
 * allocate a new directory
 */
int
allocdir(parent, request)
    ino_t parent, request;
{
    ino_t ino;
    char *cp;
    DINODE *dp;
    int st;

    ino = allocino(request, IFDIR|0755);
    dirhead.dot_ino = ino;
    dirhead.dotdot_ino = parent;
    dp = ginode(ino);
    getblk(&fileblk, dp->di_addr[0]);
    if (fileblk.b_errs != NULL) {
        freeino(ino);
        return (0);
    }
    bcopy((char *)&dirhead, dirblk.b_buf, sizeof dirhead);
    for (cp = &dirblk.b_buf[DIRBLKSIZ];
         cp < &dirblk.b_buf[DEV_BSIZE];
         cp += DIRBLKSIZ)
        bcopy((char *)&emptydir, cp, sizeof emptydir);
    dirty(&fileblk);
    dp->di_nlink = 2;
    inodirty();
    if (ino == ROOTINO) {
        setlncnt(ino, dp->di_nlink);
        return(ino);
    }
    st = getstate(parent);
    if (st != DSTATE && st != DFOUND) {
        freeino(ino);
        return (0);
    }
    setstate(ino, st);
    if (st == DSTATE) {
        setlncnt(ino, dp->di_nlink);
        inclncnt(parent);
    }
    dp = ginode(parent);
    dp->di_nlink++;
    inodirty();
    return (ino);
}
