/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Instrumentation
 */
#define CPUSTATES   4

#define CP_USER     0
#define CP_NICE     1
#define CP_SYS      2
#define CP_IDLE     3

#define DK_NDRIVE   4

#if defined(KERNEL) && defined(UCB_METER)
extern long cp_time[CPUSTATES];     /* number of ticks spent in each cpu state */
extern int  dk_ndrive;              /* number of drives being monitored */
extern int  dk_busy;                /* bit array of drive busy flags */
extern long dk_xfer[DK_NDRIVE];     /* number of transfers */
extern long dk_bytes[DK_NDRIVE];    /* number of bytes transfered */
extern char *dk_name[DK_NDRIVE];    /* names of monitored drives */
extern int  dk_unit[DK_NDRIVE];     /* unit numbers of monitored drives */
extern int  dk_n;                   /* number of dk numbers assigned so far */

extern long tk_nin;                 /* number of tty characters input */
extern long tk_nout;                /* number of tty characters output */
#endif
