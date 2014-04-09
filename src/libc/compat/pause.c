/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <signal.h>

/*
 * Backwards compatible pause.
 */
int
pause()
{
	sigset_t set;

	sigemptyset(&set);
	sigsuspend(&set);
	return 0;
}
