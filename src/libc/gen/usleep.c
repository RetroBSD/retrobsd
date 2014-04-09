/*
 * Program: sleep.c
 * Copyright: 1997, sms
 * Author: Steven M. Schultz
 *
 * Version   Date	Modification
 *     1.0  1997/9/26	1. Initial release.
 */
#include <stdio.h>	/* For NULL */
#include <sys/types.h>
#include <sys/time.h>

/*
 * This implements the usleep(3) function using only 1 system call (select)
 * instead of the 9 that the old implementation required.  Also this version
 * avoids using signals (with the attendant system overhead).
 *
 * Nothing is returned and if less than ~20000 microseconds is specified the
 * select will return without any delay at all.
 */
void
usleep(micros)
	long micros;
{
	struct timeval s;

	if (micros > 0) {
		s.tv_sec = micros / 1000000L;
		s.tv_usec = micros % 1000000L;
		select(0, NULL, NULL, NULL, &s);
	}
}
