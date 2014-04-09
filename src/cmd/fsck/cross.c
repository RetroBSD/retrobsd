/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include </usr/include/stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fstab.h>
#include "fsck.h"

char memdata [MAXDATA];

void
usage ()
{
    printf ("Usage:\n");
    printf ("    fsck [-dny] [-t filename.tmp] filesys.img\n");
    printf ("\n");
    printf ("Options:\n");
    printf ("    -d       Debug mode.\n");
    printf ("    -n       Auto answer is 'no'.\n");
    printf ("    -y       Auto answer is 'yes'.\n");
    printf ("    -t file  Filename for temporary data.\n");
}

main(argc, argv)
    int argc;
    char    *argv[];
{
    struct fstab *fsp;
    int pid, passno, anygtr, sumstatus;
    char *name, inbuf[128], outbuf[128];

    setbuffer(stdin, inbuf, sizeof (inbuf));
    setbuffer(stdout, outbuf, sizeof (outbuf));
    setlinebuf(stdout);
    sync();

    while (--argc > 0 && **++argv == '-') {
        switch (*++*argv) {

        case 't':
            if (**++argv == '-' || --argc <= 0)
                errexit("Bad -t option\n");
            strcpy(scrfile, *argv);
            break;

        case 'd':
            debug++;
            break;

        case 'n':   /* default no answer flag */
            nflag++;
            yflag = 0;
            break;

        case 'y':   /* default yes answer flag */
            yflag++;
            nflag = 0;
            break;

        default:
            errexit("%c option?\n", **argv);
        }
    }
    if (argc == 0) {
        usage();
        return 0;
    }

    memsize = sizeof (memdata);
    membase = memdata;

    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
        (void)signal(SIGINT, catch);
    if (preen)
        (void)signal(SIGQUIT, catchquit);
    while (argc-- > 0) {
        hotroot = 0;
        checkfilesys(*argv++);
    }
    return 0;
}

checkfilesys(filesys)
    char *filesys;
{
    daddr_t n_ffree, n_bfree;
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
