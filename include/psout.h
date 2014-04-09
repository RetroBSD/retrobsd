/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * psout: structure output by 'ps -U'.
 * Mostly the pre-processed /dev directory.
 */

#ifndef makedev
#include <sys/types.h>
#endif

struct psout {
	dev_t	o_ttyd;		/* u_ttyd */
	int	o_flag;		/* p_flag */
	short	o_pid;		/* p_pid */
	char	o_tty[3];	/* 1st 2 chars of tty after 'tty' */
	char	o_stat;		/* p_stat */
	short	o_uid;		/* p_uid */
	char	o_uname[UT_NAMESIZE];	/* login name of process owner */
	short	o_ppid;		/* p_ppid */
	char	o_cpu;		/* p_cpu */
	char	o_pri;		/* p_pri */
	char	o_nice;		/* p_nice */
	short	o_addr0;	/* p_addr[0] */
	short	o_size;		/* p_size */
	caddr_t	o_wchan;	/* p_wchan */
	time_t	o_utime;	/* u_utime */
	time_t	o_stime;	/* u_stime */
	time_t	o_cutime;	/* u_cutime */
	time_t	o_cstime;	/* u_cstime */
	short	o_pgrp;		/* p_pgrp */
	int	o_sigs;		/* sum of SIGINT & SIGQUIT,
				   if == 2 proc is ignoring both.*/
	char	o_comm[MAXCOMLEN+1];	/* u_comm */
	char	o_args[64];	/* best guess at args to process */
};
