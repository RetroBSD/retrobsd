/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <string.h>
#include <unistd.h>
#include <signal.h>

/*
 * Print the name of the signal indicated
 * along with the supplied message.
 */

extern	char *sys_siglist[];

void
psignal(sig, s)
	unsigned sig;
	char *s;
{
	register char *c;
	register int n;

	c = "Unknown signal";
	if (sig < NSIG)
		c = sys_siglist[sig];
	n = strlen(s);
	if (n) {
		write(2, s, n);
		write(2, ": ", 2);
	}
	write(2, c, strlen(c));
	write(2, "\n", 1);
}
