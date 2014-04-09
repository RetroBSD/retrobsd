/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/time.h>
#include <sys/types.h>

/*
 * Backwards compatible utime.
 */
int
utime(name, otv)
	char *name;
	time_t otv[];
{
	struct timeval tv[2];

	tv[0].tv_sec = otv[0]; tv[0].tv_usec = 0;
	tv[1].tv_sec = otv[1]; tv[1].tv_usec = 0;
	return utimes(name, tv);
}
