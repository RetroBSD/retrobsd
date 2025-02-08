/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "curses.ext"

/*
 *	This routine performs an insert-char on the line, leaving
 * (_cury,_curx) unchanged.
 */
int
winsch(WINDOW *win, char c)
{
	char	*temp1, *temp2;
	char	*end;

	end = &win->_y[win->_cury][win->_curx];
	temp1 = &win->_y[win->_cury][win->_maxx - 1];
	temp2 = temp1 - 1;
	while (temp1 > end)
		*temp1-- = *temp2--;
	*temp1 = c;
	touchline(win, win->_cury, win->_curx, win->_maxx - 1);
	if (win->_cury == LINES - 1 && win->_y[LINES-1][COLS-1] != ' ') {
		if (! win->_scroll)
			return ERR;
		wrefresh(win);
		scroll(win);
		win->_cury--;
        }
	return OK;
}
