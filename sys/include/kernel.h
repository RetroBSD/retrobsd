/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Global variables for the kernel
 */

/* 1.1 */
extern long hostid;
extern char hostname[];
extern int hostnamelen;

/* 1.2 */
#include <sys/time.h>

extern struct timeval boottime;
extern struct timeval time;
extern struct timezone tz;      /* XXX */
extern int adjdelta;
extern int hz;
extern int usechz;              /* # microseconds per hz */
extern int lbolt;               /* awoken once a second */

extern short avenrun[3];
