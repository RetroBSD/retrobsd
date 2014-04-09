/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "curses.ext"

/*
 * This routine adds a string starting at (_cury,_curx)
 */
int
waddstr(win, str)
        reg WINDOW	*win;
        reg char	*str;
{
# ifdef DEBUG
	fprintf(outf, "WADDSTR(\"%s\")\n", str);
# endif
	while (*str)
		if (waddch(win, *str++) == ERR)
			return ERR;
	return OK;
}
