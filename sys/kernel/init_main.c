/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "user.h"
#include "fs.h"
#include "mount.h"
#include "map.h"
#include "proc.h"
#include "ioctl.h"
#include "inode.h"
#include "conf.h"
#include "buf.h"
#include "fcntl.h"
#include "vm.h"
#include "clist.h"
#include "reboot.h"
#include "systm.h"
#include "kernel.h"
#include "namei.h"
#include "stat.h"
#include "rdisk.h"

u_int	swapstart, nswap;	/* start and size of swap space */
size_t	physmem;		/* total amount of physical memory */
int	boothowto;		/* reboot flags, from boot */

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
 *	clear and free user core
 *	turn on clock
 *	hand craft 0th process
 *	call all initialization routines
 *	fork - process 0 to schedule
 *	     - process 1 execute bootstrap
 */
int
main()
{
	register struct proc *p;
	register int i;
	register struct fs *fs = NULL;
	char inbuf[4];
	char inch;
    int s __attribute__((unused));

	startup();
	printf ("\n%s", version);
        cpuidentify();
        cnidentify();

	/*
	 * Set up system process 0 (swapper).
	 */
	p = &proc[0];
	p->p_addr = (size_t) &u;
	p->p_stat = SRUN;
	p->p_flag |= SLOAD | SSYS;
	p->p_nice = NZERO;

	u.u_procp = p;			/* init user structure */
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
        s = spl0();
	rdisk_init();

	pipedev = rootdev = get_boot_device();
	swapdev = get_swap_device();

	/* Mount a root filesystem. */
        for (;;) {
		if(rootdev!=-1)
		{
			fs = mountfs (rootdev, (boothowto & RB_RDONLY) ? MNT_RDONLY : 0,
				(struct inode*) 0);
		}
		if (fs)
			break;
		printf ("No root filesystem available!\n");
//		rdisk_list_partitions(RDISK_FS);
retry:
		printf ("Please enter device to boot from (press ? to list): ");
		inch=0;
		inbuf[0] = inbuf[1] = inbuf[2] = inbuf[3] = 0;
		while((inch=cngetc()) != '\r')
		{
			switch(inch)
			{
				case '?':
					printf("?\n");
					rdisk_list_partitions(RDISK_FS);
					printf ("Please enter device to boot from (press ? to list): ");
					break;
				default:
					printf("%c",inch);
					inbuf[0] = inbuf[1];
					inbuf[1] = inbuf[2];
					inbuf[2] = inbuf[3];
					inbuf[3] = inch;
					break;
			}
		}

		inch = 0;
		if(inbuf[0]=='r' && inbuf[1]=='d')
		{
			if(inbuf[2]>='0' && inbuf[2] < '0'+rdisk_num_disks())
			{
				if(inbuf[3]>='a' && inbuf[3]<='d')
				{
					rootdev=makedev(inbuf[2]-'0',inbuf[3]-'a'+1);
					inch = 1;
				}
			}
		} else if(inbuf[1]=='r' && inbuf[2]=='d') {
			if(inbuf[3]>='0' && inbuf[3] < '0'+rdisk_num_disks())
			{
				rootdev=makedev(inbuf[3]-'0',0);
				inch = 1;
			}
		} else if(inbuf[3] == 0) {
			inch = 1;
		}
		if(inch==0)
		{
			printf("\nUnknown device.\n\n");
			goto retry;
		}
		printf ("\n\n");
        }
	printf ("phys mem  = %u kbytes\n", physmem / 1024);
	printf ("user mem  = %u kbytes\n", MAXMEM / 1024);
	if(minor(rootdev)==0)
	{
		printf ("root dev  = rd%d (%d,%d)\n", 
			major(rootdev),
			major(rootdev), minor(rootdev)
		);
	} else {
		printf ("root dev  = rd%d%c (%d,%d)\n", 
			major(rootdev), 'a'+minor(rootdev)-1,
			major(rootdev), minor(rootdev)
		);
	}

	printf ("root size = %u kbytes\n", fs->fs_fsize * DEV_BSIZE / 1024);
	mount[0].m_inodp = (struct inode*) 1;	/* XXX */
	mount_updname (fs, "/", "root", 1, 4);
	time.tv_sec = fs->fs_time;
	boottime = time;

        /* Find a swap file. */
	swapstart = 1;
	while(swapdev == -1)
	{
		printf("Please enter swap device (press ? to list): ");
		inbuf[0] = inbuf[1] = inbuf[2] = inbuf[3] = 0;
		while((inch = cngetc())!='\r')
		{
			switch(inch)
			{
				case '?':
					printf("?\n");
					rdisk_list_partitions(RDISK_SWAP);
					printf("Please enter swap device (press ? to list): ");
					break;
				default:
					printf("%c",inch);
					inbuf[0] = inbuf[1];
					inbuf[1] = inbuf[2];
					inbuf[2] = inbuf[3];
					inbuf[3] = inch;
					break;
			}
		}
		inch = 0;
		if(inbuf[0]=='r' && inbuf[1]=='d')
		{
			if(inbuf[2]>='0' && inbuf[2] < '0'+rdisk_num_disks())
			{
				if(inbuf[3]>='a' && inbuf[3]<='d')
				{
					swapdev=makedev(inbuf[2]-'0',inbuf[3]-'a'+1);
					inch = 1;
				}
			}
		} else if(inbuf[1]=='r' && inbuf[2]=='d') {
			if(inbuf[3]>='0' && inbuf[3] < '0'+rdisk_num_disks())
			{
				swapdev=makedev(inbuf[3]-'0',0);
				inch = 1;
			}
		}

		if(minor(swapdev)!=0)
		{
			if(partition_type(swapdev)!=RDISK_SWAP)
			{
				printf("\nNot a swap partition!\n\n");
				swapdev=-1;
			}
		}
	}
	nswap = rdsize(swapdev);

	if(minor(swapdev)==0)
	{
		printf ("swap dev  = rd%d (%d,%d)\n", 
			major(swapdev),
			major(swapdev), minor(swapdev)
		);
	} else {
		printf ("swap dev  = rd%d%c (%d,%d)\n", 
			major(swapdev), 'a'+minor(swapdev)-1,
			major(swapdev), minor(swapdev)
		);
	}
        (*bdevsw[major(swapdev)].d_open)(swapdev, FREAD|FWRITE, S_IFBLK);
	printf ("swap size = %u kbytes\n", nswap * DEV_BSIZE / 1024);
	if (nswap <= 0)
		panic ("zero swap size");	/* don't want to panic, but what ? */
	mfree (swapmap, nswap, swapstart);

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
        /*
         * return goes to location 0 of user init code
         * just copied out.
         */
        return 0;
}
