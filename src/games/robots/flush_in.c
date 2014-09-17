/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <curses.h>

/*
 * flush_in:
 *	Flush all pending input.
 */
void
flush_in()
{
#ifdef TIOCFLUSH
	ioctl(fileno(stdin), TIOCFLUSH, NULL);
#else
	crmode();
#endif
}
