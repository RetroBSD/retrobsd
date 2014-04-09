/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Global variables for the kernel
 */

/* 1.1 */
long	hostid;
char	hostname[MAXHOSTNAMELEN];
int	hostnamelen;

/* 1.2 */
#include <sys/time.h>

struct	timeval boottime;
struct	timeval time;
struct	timezone tz;			/* XXX */
int	adjdelta;
int	hz;
int	usechz;				/* # microseconds per hz */
int	lbolt;				/* awoken once a second */

short	avenrun[3];
