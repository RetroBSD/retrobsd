/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Backwards compatible time call.
 */
#include <sys/types.h>
#include <sys/time.h>

long
time(t)
	time_t *t;
{
	struct timeval tt;

	if (gettimeofday(&tt, (struct timezone *)0) < 0)
		return (-1);
	if (t)
		*t = tt.tv_sec;
	return (tt.tv_sec);
}
