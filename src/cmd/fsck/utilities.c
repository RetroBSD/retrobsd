/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifdef CROSS
#include </usr/include/stdio.h>
#define off_t unsigned long long
#else
#include <stdio.h>
#include <ctype.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include <sys/ioctl.h>
#include <sys/swap.h>
#include "fsck.h"

int returntosingle;

int
ftypeok(dp)
    DINODE *dp;
{
    switch (dp->di_mode & IFMT) {

    case IFDIR:
    case IFREG:
    case IFBLK:
    case IFCHR:
    case IFLNK:
    case IFSOCK:
        return (1);

    default:
        if (debug)
            printf("bad file type 0%o\n", dp->di_mode);
        return (0);
    }
}

int
reply(s)
    char *s;
{
    char line[80];

    if (preen)
        pfatal("INTERNAL ERROR: GOT TO reply()\n");
    printf("\n%s? ", s);
    if (nflag || dfile.wfdes < 0) {
        printf(" no\n\n");
        return (0);
    }
    if (yflag) {
        printf(" yes\n\n");
        return (1);
    }
    fflush (stdout);
    if (getlin(stdin, line, sizeof(line)) == EOF)
        errexit("\n");
    printf("\n");
    if (line[0] == 'y' || line[0] == 'Y')
        return (1);
    else
        return (0);
}

int
getlin(fp, loc, maxlen)
    FILE *fp;
    char *loc;
{
    register int n;
    register char *p, *lastloc;

    p = loc;
    lastloc = &p[maxlen-1];
    while ((n = fgetc(fp)) != '\n') {
        if (n == EOF)
            return (EOF);
        if (!isspace(n) && p < lastloc)
            *p++ = n;
    }
    *p = 0;
    return (p - loc);
}

BUFAREA *
search(blk)
    daddr_t blk;
{
    register BUFAREA *pbp = 0, *bp;

    for (bp = (BUFAREA *) &poolhead; bp->b_next; ) {
        pbp = bp;
        bp = pbp->b_next;
        if (bp->b_bno == blk)
            break;
    }
    if (pbp != 0)
        pbp->b_next = bp->b_next;
    bp->b_next = poolhead;
    poolhead = bp;
    return(bp);
}

BUFAREA *
getblk(bp, blk)
    register BUFAREA *bp;
    daddr_t blk;
{
    register struct filecntl *fcp;

    if (bp == NULL) {
        bp = search(blk);
        fcp = &sfile;
    } else
        fcp = &dfile;

    if (bp->b_bno == blk)
        return(bp);
    flush(fcp,bp);
    bp->b_errs = bread(fcp,bp->b_un.b_buf,blk,DEV_BSIZE);
    bp->b_bno = blk;
    bp->b_size = DEV_BSIZE;
    return(bp);
}

void
flush(fcp, bp)
    struct filecntl *fcp;
    register BUFAREA *bp;
{

    if (!bp->b_dirty)
        return;
    if (bp->b_errs != 0)
        pfatal("WRITING ZERO'ED BLOCK %ld TO DISK\n", bp->b_bno);
    bp->b_dirty = 0;
    bp->b_errs = 0;
    if (bp == &inoblk)
        bwrite(fcp, inobuf, startib, NINOBLK * DEV_BSIZE);
    else
        bwrite(fcp, bp->b_un.b_buf, bp->b_bno, DEV_BSIZE);
}

void
rwerr(s, blk)
    char *s;
    daddr_t blk;
{
    if (preen == 0)
        printf("\n");
    pfatal("CANNOT %s: BLK %ld\n", s, blk);
    if (reply("CONTINUE") == 0)
        errexit("Program terminated\n");
}

void
ckfini()
{
    flush(&dfile, &fileblk);
    flush(&dfile, &sblk);
    if (sblk.b_bno != SUPERB) {
        sblk.b_bno = SUPERB;
        sbdirty();
        flush(&dfile, &sblk);
    }
    flush(&dfile, &inoblk);
    if (dfile.rfdes)
        (void)close(dfile.rfdes);
    if (dfile.wfdes) {
    int allocd = 0;
    ioctl(sfile.wfdes, TFALLOC, &allocd);
        (void)close(dfile.wfdes);
    }
    if (sfile.rfdes)
        (void)close(sfile.rfdes);
    if (sfile.wfdes)
        (void)close(sfile.wfdes);
}

int
bread(fcp, buf, blk, size)
    register struct filecntl *fcp;
    char *buf;
    daddr_t blk;
    int size;
{
    char *cp;
    register int i, errs;
    off_t offset;

    offset = ((off_t) blk << DEV_BSHIFT) + fcp->offset;
    if (lseek(fcp->rfdes, offset, 0) != offset)
        rwerr("SEEK", blk);
    else if (read(fcp->rfdes, buf, size) == size) {
        /*printf(".%u ", (unsigned) blk); fflush (stdout);*/
        return (0);
    }
    printf("fd: %d (sfile = %d, dfile = %d)\n",fcp->rfdes,sfile.rfdes,dfile.rfdes);
    rwerr("READ", blk);
    if (lseek(fcp->rfdes, offset, 0) != offset)
        rwerr("SEEK", blk);
    errs = 0;
    pfatal("THE FOLLOWING SECTORS COULD NOT BE READ:\n");
    for (cp = buf, i = 0; i < size; i += DEV_BSIZE, cp += DEV_BSIZE) {
        if (read(fcp->rfdes, cp, DEV_BSIZE) < 0) {
            printf(" %ld,", blk + i / DEV_BSIZE);
            bzero(cp, DEV_BSIZE);
            errs++;
        }
    }
    printf("\n");
    return (errs);
}

void
bwrite(fcp, buf, blk, size)
    register struct filecntl *fcp;
    char *buf;
    daddr_t blk;
    int size;
{
    int i;
    char *cp;
    off_t offset;

    if (fcp->wfdes < 0)
        return;
    offset = ((off_t) blk << DEV_BSHIFT) + fcp->offset;
    if (lseek(fcp->wfdes, offset, 0) != offset)
        rwerr("SEEK", blk);
    else if (write(fcp->wfdes, buf, size) == size) {
        fcp->mod = 1;
        /*printf("#%u ", (unsigned) blk); fflush (stdout);*/
        return;
    }
    rwerr("WRITE", blk);
    if (lseek(fcp->wfdes, offset, 0) != offset)
        rwerr("SEEK", blk);
    pfatal("THE FOLLOWING SECTORS COULD NOT BE WRITTEN:\n");
    for (cp = buf, i = 0; i < size; i += DEV_BSIZE, cp += DEV_BSIZE)
        if (write(fcp->wfdes, cp, DEV_BSIZE) < 0)
            printf(" %ld,", blk + i / DEV_BSIZE);
    printf("\n");
    return;
}

/*
 * allocate a data block
 */
daddr_t
allocblk()
{
    daddr_t i;

    for (i = 0; i < fsmax; i++) {
        if (getbmap(i))
            continue;
        setbmap(i);
        n_blks++;
        return (i);
    }
    return (0);
}

/*
 * Find a pathname
 */
void
getpathname(namebuf, curdir, ino)
    char *namebuf;
    ino_t curdir, ino;
{
    int len, st;
    register char *cp;
    struct inodesc idesc;

    st = getstate(ino);
    if (st != DSTATE && st != DFOUND) {
        strcpy(namebuf, "?");
        return;
    }
    bzero(&idesc, sizeof(struct inodesc));
    idesc.id_type = DATA;
    cp = &namebuf[MAXPATHLEN - 1];
    *cp-- = '\0';
    if (curdir != ino) {
        idesc.id_parent = curdir;
        goto namelookup;
    }
    while (ino != ROOTINO) {
        idesc.id_number = ino;
        idesc.id_func = findino;
        idesc.id_name = "..";
        if ((ckinode(ginode(ino), &idesc) & STOP) == 0)
            break;
    namelookup:
        idesc.id_number = idesc.id_parent;
        idesc.id_parent = ino;
        idesc.id_func = findname;
        idesc.id_name = namebuf;
        if ((ckinode(ginode(idesc.id_number), &idesc) & STOP) == 0)
            break;
        len = strlen(namebuf);
        cp -= len;
        if (cp < &namebuf[MAXNAMLEN])
            break;
        bcopy(namebuf, cp, len);
        *--cp = '/';
        ino = idesc.id_number;
    }
    if (ino != ROOTINO) {
        strcpy(namebuf, "?");
        return;
    }
    bcopy(cp, namebuf, &namebuf[MAXPATHLEN] - cp);
}

void
catch (sig)
{
    ckfini();
    exit(12);
}

/*
 * When preening, allow a single quit to signal
 * a special exit after filesystem checks complete
 * so that reboot sequence may be interrupted.
 */
void
catchquit (sig)
{
    printf("returning to single-user after filesystem check\n");
    returntosingle = 1;
    (void)signal(SIGQUIT, SIG_DFL);
}

/*
 * Ignore a single quit signal; wait and flush just in case.
 * Used by child processes in preen.
 */
void
voidquit (sig)
{

    sleep(1);
    (void)signal(SIGQUIT, SIG_IGN);
    (void)signal(SIGQUIT, SIG_DFL);
}

/*
 * determine whether an inode should be fixed.
 */
int
dofix(idesc, msg)
    register struct inodesc *idesc;
    char *msg;
{

    switch (idesc->id_fix) {

    case DONTKNOW:
        if (idesc->id_type == DATA)
            direrr(idesc->id_number, msg);
        else
            pwarn(msg);
        if (preen) {
            printf(" (SALVAGED)\n");
            idesc->id_fix = FIX;
            return (ALTERED);
        }
        if (reply("SALVAGE") == 0) {
            idesc->id_fix = NOFIX;
            return (0);
        }
        idesc->id_fix = FIX;
        return (ALTERED);

    case FIX:
        return (ALTERED);

    case NOFIX:
        return (0);

    default:
        errexit("UNKNOWN INODESC FIX MODE %d\n", idesc->id_fix);
    }
    /* NOTREACHED */
    return (0);
}

void
errexit(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    exit(8);
}

/*
 * An inconsistency occured which shouldn't during normal operations.
 * Die if preening, otherwise just printf.
 */
void
pfatal(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    if (preen) {
        printf("%s: ", devnam);
        vprintf(fmt, ap);
        va_end(ap);
        printf("\n");
        printf("%s: UNEXPECTED INCONSISTENCY; RUN fsck MANUALLY.\n",
            devnam);
        exit(8);
    }
    vprintf(fmt, ap);
    va_end(ap);
}

/*
 * Pwarn is like printf when not preening,
 * or a warning (preceded by filename) when preening.
 */
void
pwarn(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    if (preen)
        printf("%s: ", devnam);
    vprintf(fmt, ap);
    va_end(ap);
}

int
dostate(ino, s,flg)
    ino_t ino;
    int s, flg;
{
    register char *p;
    register unsigned byte, shift;
    BUFAREA *bp;

    byte = ino / STATEPB;
    shift = LSTATE * (ino % STATEPB);
    if (statemap != NULL) {
        bp = NULL;
        p = &statemap[byte];
    } else {
        bp = getblk(NULL,(daddr_t)(smapblk+(byte/DEV_BSIZE)));
        if (bp->b_errs)
            errexit("Fatal I/O error\n");
        p = &bp->b_un.b_buf[byte%DEV_BSIZE];
    }
    switch (flg) {
    case 0:
        *p &= ~(SMASK<<(shift));
        *p |= s << shift;
        if (bp != NULL)
            dirty(bp);
        return(s);
    case 1:
        return((*p >> shift) & SMASK);
    }
    return(USTATE);
}

int
domap(blk, flg)
    daddr_t blk;
    int flg;
{
    register char *p;
    register unsigned n;
    register BUFAREA *bp;
    off_t byte;

    byte = blk >> BITSHIFT;
    n = 1<<((unsigned)(blk & BITMASK));
    if (flg & 04) {
        p = freemap;
        blk = fmapblk;
    } else {
        p = blockmap;
        blk = bmapblk;
    }
    if (p != NULL) {
        bp = NULL;
        p += (unsigned)byte;
    } else {
        bp = getblk((BUFAREA *)NULL,blk+(byte>>DEV_BSHIFT));
        if (bp->b_errs)
            errexit("Fatal I/O error\n");
        p = &bp->b_un.b_buf[(unsigned)(byte&DEV_BMASK)];
    }
    switch (flg&03) {
    case 0:
        *p |= n;
        break;
    case 1:
        n &= *p;
        bp = NULL;
        break;
    case 2:
        *p &= ~n;
    }
    if (bp != NULL)
        dirty(bp);
    return(n);
}

int
dolncnt(ino, flg, val)
    ino_t ino;
    short val, flg;
{
    register short *sp;
    register BUFAREA *bp;

    if (lncntp != NULL) {
        bp = NULL;
        sp = &lncntp[ino];
    } else {
        bp = getblk((BUFAREA *)NULL,(daddr_t)(lncntblk+(ino/SPERB)));
        if (bp->b_errs)
            errexit("Fatal I/O error\n");
        sp = &bp->b_un.b_lnks[ino%SPERB];
    }
    switch (flg) {
    case 0:
        *sp = val;
        break;
    case 1:
        bp = NULL;
        break;
    case 2:
        (*sp)--;
        break;
    case 3:
        (*sp)++;
        break;
    default:
        abort();
    }
    if (bp != NULL)
        dirty(bp);
    return(*sp);
}
