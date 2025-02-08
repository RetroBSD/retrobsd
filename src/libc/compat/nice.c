/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/time.h>
#include <sys/resource.h>

/*
 * Backwards compatible nice.
 */
int
nice(int incr)
{
	int prio;
	extern int errno;

	errno = 0;
	prio = getpriority(PRIO_PROCESS, 0);
	if (prio == -1 && errno)
		return (-1);
	return setpriority(PRIO_PROCESS, 0, prio + incr);
}
