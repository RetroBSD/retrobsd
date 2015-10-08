/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/param.h>
#include <sys/user.h>
#include <sys/fs.h>
#include <sys/mount.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/inode.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/fcntl.h>
#include <sys/vm.h>
#include <sys/clist.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/namei.h>
#include <sys/stat.h>
#include <sys/kconfig.h>

u_int   swapstart, nswap;   /* start and size of swap space */
size_t  physmem;            /* total amount of physical memory */
int     boothowto;          /* reboot flags, from boot */

/*
 * Initialize hash links for buffers.
 */
static void
bhinit()
{
    register int i;
    register struct bufhd *bp;

    for (bp = bufhash, i = 0; i < BUFHSZ; i++, bp++)
        bp->b_forw = bp->b_back = (struct buf *)bp;
}

/*
 * Initialize the buffer I/O system by freeing
 * all buffers and setting all device buffer lists to empty.
 */
static void
binit()
{
    register struct buf *bp;
    register int i;
    caddr_t paddr;

    for (bp = bfreelist; bp < &bfreelist[BQUEUES]; bp++)
        bp->b_forw = bp->b_back = bp->av_forw = bp->av_back = bp;
    paddr = bufdata;
    for (i = 0; i < NBUF; i++, paddr += MAXBSIZE) {
        bp = &buf[i];
        bp->b_dev = NODEV;
        bp->b_bcount = 0;
        bp->b_addr = paddr;
        binshash(bp, &bfreelist[BQ_AGE]);
        bp->b_flags = B_BUSY|B_INVAL;
        brelse(bp);
    }
}

/*
 * Initialize clist by freeing all character blocks, then count
 * number of character devices. (Once-only routine)
 */
static void
cinit()
{
    register int ccp;
    register struct cblock *cp;

    ccp = (int)cfree;
    ccp = (ccp + CROUND) & ~CROUND;
    for (cp = (struct cblock *)ccp; cp <= &cfree[NCLIST - 1]; cp++) {
        cp->c_next = cfreelist;
        cfreelist = cp;
        cfreecount += CBSIZE;
    }
}

/*
 * Initialization code.
 * Called from cold start routine as
 * soon as a stack and segmentation
 * have been established.
 * Functions:
 *  clear and free user core
 *  turn on clock
 *  hand craft 0th process
 *  call all initialization routines
 *  fork - process 0 to schedule
 *       - process 1 execute bootstrap
 */
int
main()
{
    register struct proc *p;
    register int i;
    register struct fs *fs = NULL;
    int s __attribute__((unused));

    startup();
    printf ("\n%s", version);
    kconfig();

    /*
     * Set up system process 0 (swapper).
     */
    p = &proc[0];
    p->p_addr = (size_t) &u;
    p->p_stat = SRUN;
    p->p_flag |= SLOAD | SSYS;
    p->p_nice = NZERO;

    u.u_procp = p;          /* init user structure */
    u.u_cmask = CMASK;
    u.u_lastfile = -1;
    for (i = 1; i < NGROUPS; i++)
        u.u_groups[i] = NOGROUP;
    for (i = 0; i < sizeof(u.u_rlimit)/sizeof(u.u_rlimit[0]); i++)
        u.u_rlimit[i].rlim_cur = u.u_rlimit[i].rlim_max =
            RLIM_INFINITY;

    /* Initialize signal state for process 0 */
    siginit (p);

    /*
     * Initialize tables, protocols, and set up well-known inodes.
     */
#ifdef LOG_ENABLED
    loginit();
#endif
    coutinit();
    cinit();
    pqinit();
    ihinit();
    bhinit();
    binit();
    nchinit();
    clkstart();

    pipedev = rootdev;

    /* Attach services. */
    struct conf_service *svc;
    for (svc = conf_service_init; svc->svc_attach != NULL; svc++)
        (*svc->svc_attach)();

    /* Mount a root filesystem. */
    s = spl0();
    fs = mountfs (rootdev, (boothowto & RB_RDONLY) ? MNT_RDONLY : 0, 0);
    if (! fs)
        panic ("No root filesystem found!");
    mount[0].m_inodp = (struct inode*) 1;   /* XXX */
    mount_updname (fs, "/", "root", 1, 4);
    time.tv_sec = fs->fs_time;
    boottime = time;

    /* Find a swap file. */
    swapstart = 1;
    (*bdevsw[major(swapdev)].d_open)(swapdev, FREAD|FWRITE, S_IFBLK);
    nswap = (*bdevsw[major(swapdev)].d_psize)(swapdev);
    if (nswap <= 0)
        panic ("zero swap size");   /* don't want to panic, but what ? */
    mfree (swapmap, nswap, swapstart);

    printf ("phys mem  = %u kbytes\n", physmem / 1024);
    printf ("user mem  = %u kbytes\n", MAXMEM / 1024);
    printf ("root dev  = (%d,%d)\n", major(rootdev), minor(rootdev));
    printf ("swap dev  = (%d,%d)\n", major(swapdev), minor(swapdev));
    printf ("root size = %u kbytes\n", fs->fs_fsize * DEV_BSIZE / 1024);
    printf ("swap size = %u kbytes\n", nswap * DEV_BSIZE / 1024);

    /* Kick off timeout driven events by calling first time. */
    schedcpu (0);

    /* Set up the root file system. */
    rootdir = iget (rootdev, &mount[0].m_filsys, (ino_t) ROOTINO);
    iunlock (rootdir);
    u.u_cdir = iget (rootdev, &mount[0].m_filsys, (ino_t) ROOTINO);
    iunlock (u.u_cdir);
    u.u_rdir = NULL;

    /*
     * Make init process.
     */
    if (newproc (0) == 0) {
        /* Parent process with pid 0: swapper.
         * No return from sched. */
        sched();
    }

    /* Child process with pid 1: init. */
    s = splhigh();
    p = u.u_procp;
    p->p_dsize = icodeend - icode;
    p->p_daddr = USER_DATA_START;
    p->p_ssize = 1024;              /* one kbyte of stack */
    p->p_saddr = USER_DATA_END - 1024;
    bcopy ((caddr_t) icode, (caddr_t) USER_DATA_START, icodeend - icode);

    /* Start in single user more, if asked. */
    if (boothowto & RB_SINGLE) {
        char *iflags = (char*) USER_DATA_START + (initflags - icode);

        /* Call /sbin/init with option '-s'. */
        iflags[1] = 's';
    }

    /*
     * return goes to location 0 of user init code
     * just copied out.
     */
    return 0;
}
