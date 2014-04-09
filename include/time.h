/*
 * Copyright (c) 1983, 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifndef _TIME_H
#define _TIME_H

#include <sys/types.h>		/* for time_t */

/*
 * Structure returned by gmtime and localtime calls (see ctime(3)).
 */
struct tm {
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
	long	tm_gmtoff;
	char	*tm_zone;
};

extern	struct tm *gmtime(), *localtime();
extern	char *asctime(), *ctime();
extern	time_t	time();

size_t strftime (char *s, size_t maxsize, const char *format,
	const struct tm *timeptr);

#endif
