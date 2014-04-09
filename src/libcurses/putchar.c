/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "curses.ext"

int
_putchar(c)
        reg int	c;
{
	putchar(c);
#ifdef DEBUG
	fprintf(outf, "_PUTCHAR(%s)\n", unctrl(c));
#endif
        return 0;
}
