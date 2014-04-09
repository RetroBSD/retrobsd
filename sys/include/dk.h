/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Instrumentation
 */
#define	CPUSTATES	4

#define	CP_USER		0
#define	CP_NICE		1
#define	CP_SYS		2
#define	CP_IDLE		3

#define	DK_NDRIVE	4

#if defined(KERNEL) && defined(UCB_METER)
long	cp_time[CPUSTATES];	/* number of ticks spent in each cpu state */
int	dk_ndrive;		/* number of drives being monitored */
int	dk_busy;		/* bit array of drive busy flags */
long	dk_xfer[DK_NDRIVE];	/* number of transfers */
long	dk_bytes[DK_NDRIVE];	/* number of bytes transfered */
char	*dk_name[DK_NDRIVE];	/* names of monitored drives */
int	dk_unit[DK_NDRIVE];	/* unit numbers of monitored drives */
int	dk_n;			/* number of dk numbers assigned so far */

long	tk_nin;			/* number of tty characters input */
long	tk_nout;		/* number of tty characters output */
#endif
