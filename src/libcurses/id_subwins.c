/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "curses.ext"

/*
 * _id_subwins:
 *	Re-sync the pointers to _y for all the subwindows.
 */
void
_id_subwins(WINDOW *orig)
{
	WINDOW	*win;
	int	realy;
	int	y, oy;

	realy = orig->_begy + orig->_cury;
	for (win = orig->_nextp; win != orig; win = win->_nextp) {
		/*
		 * If the window ends before our current position,
		 * don't need to do anything.
		 */
		if (win->_begy + win->_maxy <= realy)
			continue;

		oy = orig->_cury;
		for (y = realy - win->_begy; y < win->_maxy; y++, oy++)
			win->_y[y] = &orig->_y[oy][win->_ch_off];
	}
}
