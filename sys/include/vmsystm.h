/*
 * Miscellaneous virtual memory subsystem variables and structures.
 *
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *  @(#)vmsystm.h   7.2.1 (2.11BSD GTE) 1/15/95
 */

/*
 * Fork/vfork accounting.
 */
struct  forkstat
{
    long    cntfork;
    long    cntvfork;
    long    sizfork;
    long    sizvfork;
};

#if defined(KERNEL) && defined(UCB_METER)
extern size_t  freemem;     /* remaining clicks of free memory */

extern u_short avefree;     /* moving average of remaining free clicks */
extern u_short avefree30;   /* 30 sec (avefree is 5 sec) moving average */

/*
 * writable copies of tunables
 */
extern int maxslp;          /* max sleep time before very swappable */

extern struct forkstat forkstat;
#endif
