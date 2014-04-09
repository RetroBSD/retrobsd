/*
 * printw and friends
 *
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "curses.ext"

/*
 * This routine actually executes the printf and adds it to the window
 *
 * This is really a modified version of "sprintf".  As such,
 * it assumes that sprintf interfaces with the other printf functions
 * in a certain way.  If this is not how your system works, you
 * will have to modify this routine to use the interface that your
 * "sprintf" uses.
 */
int _sprintw (WINDOW *win, char *fmt, va_list args)
{
	FILE	junk;
	char	buf[512];

	junk._flag = _IOWRT + _IOSTRG;
	junk._ptr = buf;
	junk._cnt = 32767;
	_doprnt(fmt, args, &junk);
	putc('\0', &junk);
	return waddstr(win, buf);
}

/*
 * This routine implements a printf on the standard screen.
 */
int printw (char *fmt, ...)
{
	va_list args;
        int ret;

	va_start (args, fmt);
	ret = _sprintw (stdscr, fmt, args);
	va_end (args);
	return ret;
}

/*
 * This routine implements a printf on the given window.
 */
int wprintw (WINDOW *win, char *fmt, ...)
{
	va_list args;
        int ret;

	va_start (args, fmt);
	ret = _sprintw (win, fmt, &args);
	va_end (args);
	return ret;
}
