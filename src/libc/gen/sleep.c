/*
 * Program: sleep.c
 * Copyright: 1997, sms
 * Author: Steven M. Schultz
 *
 * Version   Date	Modification
 *     1.0  1997/9/25	1. Initial release.
 */

#include <stdio.h>	/* For NULL */
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

/*
 * This implements the sleep(3) function using only 3 system calls instead of
 * the 9 that the old implementation required.  Also this version avoids using
 * signals (with the attendant system overhead) and returns the amount of
 * time left unslept if an interrupt occurs.
 *
 * The error status of gettimeofday is not checked because if that fails the
 * program has scrambled the stack so badly that a sleep() failure is the least
 * problem the program has.  The select() call either completes successfully
 * or is interrupted - no errors to be checked for.
 */
u_int
sleep(seconds)
	u_int seconds;
{
	struct timeval f, s;

	if (seconds) {
		gettimeofday(&f, NULL);
		s.tv_sec = seconds;
		s.tv_usec = 0;
		select(0, NULL, NULL, NULL, &s);
		gettimeofday(&s, NULL);
		seconds -= (s.tv_sec - f.tv_sec);
/*
 * ONLY way this can happen is if the system time gets set back while we're
 * in the select() call.  In this case return 0 instead of a bogus number.
 */
		if (seconds < 0)
			seconds = 0;
	}
	return(seconds);
}
