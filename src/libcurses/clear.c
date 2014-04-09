/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "curses.ext"

/*
 * This routine clears the window.
 */
int wclear(win)
        reg WINDOW *win;
{
	werase(win);
	win->_clear = TRUE;
	return OK;
}
