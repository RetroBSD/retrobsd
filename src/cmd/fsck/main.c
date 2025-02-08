/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifdef CROSS
#include </usr/include/stdio.h>
#else
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fstab.h>
#include <pwd.h>
#include <paths.h>
#include "fsck.h"

extern  int returntosingle;

static char memdata[16 * sizeof(BUFAREA)];

BUFAREA inoblk;         /* inode blocks */
BUFAREA fileblk;        /* other blks in filesys */
BUFAREA sblk;           /* file system superblock */
BUFAREA *poolhead;      /* ptr to first buffer in pool */
struct filecntl dfile, sfile;    /* file descriptors for filesys/scratch files */
daddr_t duplist[DUPTBLSIZE];    /* head of dup list */
daddr_t *enddup;                /* next entry in dup table */
daddr_t *muldup;                /* multiple dups part of table */
ino_t   zlnlist[MAXLNCNT];      /* zero link count table */
ino_t   *zlnp;
char    inobuf[NINOBLK*INOPB*sizeof (struct dinode)];   /* allocate now */
daddr_t startib;
unsigned int memsize;
char    *devnam;
char    nflag;                  /* assume a no response */
char    yflag;                  /* assume a yes response */
char    sflag;                  /* rebuild free list */
int     debug;                  /* output debugging info */
char    preen;                  /* just fix normal inconsistencies */
char    hotroot;                /* checking root device */
char    fixfree;                /* force rebuild of freelist */
char    *membase;               /* base of memory we get */
char    *blockmap;              /* ptr to primary blk allocation map */
char    *freemap;               /* ptr to secondary blk allocation map */
char    *statemap;              /* ptr to inode state table */
short   *lncntp;                /* ptr to link count table */
char    pathname[MAXPATHLEN];   /* current pathname */
char    scrfile[80];            /* scratchfile name */
char    *pathp;                 /* pointer to pathname position */
daddr_t fsmin;                  /* block number of the first data block */
daddr_t fsmax;                  /* number of blocks in the volume */
ino_t   imax;                   /* number of inodes */
ino_t   lastino;                /* hiwater mark of inodes */
ino_t   lfdir;                  /* lost & found directory inode number */
off_t   bmapsz;                 /* num chars in blockmap */
daddr_t bmapblk;                /* starting blk of block map */
daddr_t smapblk;                /* starting blk of state map */
daddr_t lncntblk;               /* starting blk of link cnt table */
daddr_t fmapblk;                /* starting blk of free map */
daddr_t n_blks;                 /* number of blocks used */
daddr_t n_files;                /* number of files seen */
daddr_t n_free;                 /* number of free blocks */
int badblk, dupblk;
struct  dinode  zino;

char *
unrawname(cp)
    char *cp;
{
    char *dp = rindex(cp, '/');
    struct stat stb;

    if (dp == 0)
        return (cp);
    if (stat(cp, &stb) < 0)
        return (cp);
    if ((stb.st_mode&S_IFMT) != S_IFCHR)
        return (cp);
    if (*(dp+1) != 'r')
        return (cp);
    (void)strcpy(dp+1, dp+2);
    return (cp);
}

char *
rawname(cp)
    char *cp;
{
    static char rawbuf[32];
    char *dp = rindex(cp, '/');

    if (dp == 0)
        return (0);
    *dp = 0;
    (void)strcpy(rawbuf, cp);
    *dp = '/';
    (void)strcat(rawbuf, "/r");
    (void)strcat(rawbuf, dp+1);
    return (rawbuf);
}

char *
blockcheck(name)
    char *name;
{
    return name;
    struct stat stslash, stblock, stchar;
    char *raw;
    int looped = 0;

    hotroot = 0;
    if (stat("/", &stslash) < 0){
        printf("Can't stat root\n");
        return (0);
    }
retry:
    if (stat(name, &stblock) < 0){
        printf("Can't stat %s\n", name);
        return (0);
    }
    if (stblock.st_mode & S_IFBLK) {
        raw = rawname(name);
        if (stat(raw, &stchar) < 0){
            printf("Can't stat %s\n", raw);
            return (0);
        }
        if (! (stchar.st_mode & S_IFCHR)) {
            printf("%s is not a character device\n", raw);
            return (0);
        }
        if (stslash.st_dev == stblock.st_rdev) {
            hotroot++;
            raw = unrawname(name);
        }
        return (raw);
    } else if (stblock.st_mode & S_IFCHR) {
        if (looped) {
            printf("Can't make sense out of name %s\n", name);
            return (0);
        }
        name = unrawname(name);
        looped++;
        goto retry;
    }
    printf("Can't make sense out of name %s\n", name);
    return (0);
}

void
checkfilesys(filesys)
    char *filesys;
{
    register ino_t *zp;

    devnam = filesys;
    if (setup(filesys) == 0) {
        if (preen)
            pfatal("CAN'T CHECK FILE SYSTEM.\n");
        return;
    }
    /*
     * 1: scan inodes tallying blocks used
     */
    if (preen == 0) {
        printf("** Last Mounted on %s\n", sblock.fs_fsmnt);
        if (hotroot)
            printf("** Root file system\n");
        printf("** Phase 1 - Check Blocks and Sizes\n");
    }
    pass1();

    /*
     * 1b: locate first references to duplicates, if any
     */
    if (enddup != duplist) {
        if (preen)
            pfatal("INTERNAL ERROR: dups with -p\n");
        printf("** Phase 1b - Rescan For More DUPS\n");
        pass1b();
    }

    /*
     * 2: traverse directories from root to mark all connected directories
     */
    if (preen == 0)
        printf("** Phase 2 - Check Pathnames\n");
    pass2();

    /*
     * 3: scan inodes looking for disconnected directories
     */
    if (preen == 0)
        printf("** Phase 3 - Check Connectivity\n");
    pass3();

    /*
     * 4: scan inodes looking for disconnected files; check reference counts
     */
    if (preen == 0)
        printf("** Phase 4 - Check Reference Counts\n");
    pass4();

    flush(&dfile, &fileblk);

    /*
     * 5: check and repair resource counts in cylinder groups
     */
    if (preen == 0)
        printf("** Phase 5 - Check Free List\n");
    pass5();

    /*
     * print out summary statistics
     */
    pwarn("%ld files, %ld used, %ld free\n",
        n_files, n_blks, sblock.fs_tfree);
    if (debug && (n_files -= imax - ROOTINO - sblock.fs_tinode))
        printf("%ld files missing, imax: %u tinode: %u\n", n_files,
            imax,sblock.fs_tinode);
    if (debug) {
        if (enddup != duplist) {
            printf("The following duplicate blocks remain:");
            for (; enddup > duplist; enddup--)
                printf(" %ld,", *enddup);
            printf("\n");
        }
        for (zp = zlnlist; zp < zlnp; zp++)
            if (*zp) break;
        if (zp < zlnp) {
            printf("The following zero link count inodes remain:");
            for (zp = zlnlist; zp < zlnp; zp++)
                if (*zp) printf(" %lu,", (unsigned long) *zp);
            printf("\n");
        }
    }
    bzero(zlnlist, sizeof zlnlist);
    bzero(duplist, sizeof duplist);
    if (dfile.mod) {
        (void)time(&sblock.fs_time);
        sbdirty();
    }
    ckfini();
    if (!dfile.mod)
        return;
    if (!preen) {
        printf("\n***** FILE SYSTEM WAS MODIFIED *****\n");
        if (hotroot)
            printf("\n***** REBOOT UNIX *****\n");
    }
    if (hotroot) {
        sync();
        exit(4);
    }
}

int
main(argc, argv)
    int argc;
    char    *argv[];
{
    struct fstab *fsp;
    int pid, passno, anygtr, sumstatus;
    char *name, inbuf[64], outbuf[64], errbuf[64];
    extern void _start();

    setvbuf(stdin, inbuf, _IOFBF, sizeof (inbuf));
    setvbuf(stdout, outbuf, _IOLBF, sizeof (outbuf));
    setvbuf(stderr, errbuf, _IOLBF, sizeof (errbuf));
    sync();

    while (--argc > 0 && **++argv == '-') {
        switch (*++*argv) {

        case 't':
        case 'T':
            if (**++argv == '-' || --argc <= 0)
                errexit("Bad -t option\n");
            strcpy(scrfile, *argv);
            break;

        case 'p':
            preen++;
            break;

        case 'd':
            debug++;
            break;

        case 'n':   /* default no answer flag */
        case 'N':
            nflag++;
            yflag = 0;
            break;

        case 'y':   /* default yes answer flag */
        case 'Y':
            yflag++;
            nflag = 0;
            break;

        default:
            errexit("%c option?\n", **argv);
        }
    }

    /*
     * fsck has a problem under a 4BSD C library in that if its done
     * its sbrk's before it accesses FSTAB, there's no space left
     * for stdio.  There may be other problems like this.  Enjoy.
     */
    if (!getfsent())
        errexit("Can't open checklist file: %s\n", _PATH_FSTAB);
    setpassent(1);

    membase = memdata;
    memsize = sizeof(memdata);

    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
        (void)signal(SIGINT, catch);
    if (preen)
        (void)signal(SIGQUIT, catchquit);
    if (argc) {
        while (argc-- > 0) {
            hotroot = 0;
            checkfilesys(*argv++);
        }
        exit(0);
    }
    sumstatus = 0;
    passno = 1;
    do {
        anygtr = 0;
        if (setfsent() == 0)
            errexit("Can't open %s\n", _PATH_FSTAB);
        while ((fsp = getfsent()) != 0) {
            if (strcmp(fsp->fs_vfstype, "ufs") ||
                (strcmp(fsp->fs_type, FSTAB_RW) &&
                strcmp(fsp->fs_type, FSTAB_RO) &&
                strcmp(fsp->fs_type, FSTAB_RQ)) ||
                fsp->fs_passno == 0)
                continue;
            if (preen == 0 ||
                (passno == 1 && fsp->fs_passno == passno)) {
                name = blockcheck(fsp->fs_spec);
                if (name != NULL)
                    checkfilesys(name);
                else if (preen)
                    exit(8);
            } else if (fsp->fs_passno > passno) {
                anygtr = 1;
            } else if (fsp->fs_passno == passno) {
                pid = fork();
                if (pid < 0) {
                    perror("fork");
                    exit(8);
                }
                if (pid == 0) {
                    (void)signal(SIGQUIT, voidquit);
                    name = blockcheck(fsp->fs_spec);
                    if (name == NULL)
                        exit(8);
                    checkfilesys(name);
                    exit(0);
                }
            }
        }
        if (preen) {
            int status;
            while (wait(&status) != -1)
                sumstatus |= WEXITSTATUS(status);
        }
        passno++;
    } while (anygtr);

    if (sumstatus)
        exit(8);
    (void)endfsent();
    if (returntosingle)
        exit(2);
    exit(0);
}
