/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef	_SYS_TIME_H_
#define	_SYS_TIME_H_

#include <sys/types.h>

/*
 * Structure returned by gettimeofday(2) system call,
 * and used in other calls.
 */
struct timeval {
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* and microseconds */
};

/*
 * Structure defined by POSIX.4 to be like a timeval but with nanoseconds
 * instead of microseconds.  Silly on a PDP-11 but keeping the names the
 * same makes life simpler than changing the names.
*/
struct timespec {
	time_t tv_sec;		/* seconds */
	long   tv_nsec;		/* and nanoseconds */
};

struct timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of dst correction */
};
#define	DST_NONE	0	/* not on dst */
#define	DST_USA		1	/* USA style dst */
#define	DST_AUST	2	/* Australian style dst */
#define	DST_WET		3	/* Western European dst */
#define	DST_MET		4	/* Middle European dst */
#define	DST_EET		5	/* Eastern European dst */
#define	DST_CAN		6	/* Canada */

/*
 * Operations on timevals.
 *
 * NB: timercmp does not work for >= or <=.
 */
#define	timerisset(tvp)		((tvp)->tv_sec || (tvp)->tv_usec)
#define	timercmp(tvp, uvp, cmp)	\
	((tvp)->tv_sec cmp (uvp)->tv_sec || \
	 (tvp)->tv_sec == (uvp)->tv_sec && (tvp)->tv_usec cmp (uvp)->tv_usec)
#define	timerclear(tvp)		(tvp)->tv_sec = (tvp)->tv_usec = 0

/*
 * Names of the interval timers, and structure
 * defining a timer setting.
 */
#define	ITIMER_REAL	0
#define	ITIMER_VIRTUAL	1
#define	ITIMER_PROF	2

struct	k_itimerval {
	long	it_interval;		/* timer interval */
	long	it_value;		/* current value */
};

struct	itimerval {
	struct	timeval it_interval;	/* timer interval */
	struct	timeval it_value;	/* current value */
};

#ifdef KERNEL
/*
 * Round up a proposed time value to a minimal resolution of the clock.
 */
int itimerfix (struct timeval *tv);

/*
 * Add and subtract routines for timevals.
 */
void timevaladd (struct timeval *t1, struct timeval *t2);
void timevalsub (struct timeval *t1, struct timeval *t2);

/*
 * Compute number of hz until specified time.
 */
int hzto (struct timeval *tv);

#else
#include <time.h>

int gettimeofday (struct timeval *tv, struct timezone *tz);
int utimes (const char *filename, const struct timeval times[2]);
int getitimer (int which, struct itimerval *curr_value);
int setitimer (int which, const struct itimerval *new_value,
               struct itimerval *old_value);
int getpriority (int which, int who);
int setpriority (int which, int who, int prio);
char *tztab (int zone, int dst);

#endif

/*
 * Getkerninfo clock information structure
 */
struct clockinfo {
	int	hz;		/* clock frequency */
	int	tick;		/* micro-seconds per hz tick */
	int	stathz;		/* statistics clock frequency */
	int	profhz;		/* profiling clock frequency */
};

extern unsigned int msec();
#endif	/* !_SYS_TIME_H_ */
