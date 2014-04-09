/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/times.h>

static long
scale60(tvp)
	register struct timeval *tvp;
{
	return (tvp->tv_sec * 60 + tvp->tv_usec / 16667);
}

int
times(tmsp)
	register struct tms *tmsp;
{
	struct rusage ru;
	long scale60();

	if (getrusage(RUSAGE_SELF, &ru) < 0)
		return (-1);
	tmsp->tms_utime = scale60(&ru.ru_utime);
	tmsp->tms_stime = scale60(&ru.ru_stime);
	if (getrusage(RUSAGE_CHILDREN, &ru) < 0)
		return (-1);
	tmsp->tms_cutime = scale60(&ru.ru_utime);
	tmsp->tms_cstime = scale60(&ru.ru_stime);
	return (0);
}
