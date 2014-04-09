/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "curses.ext"

/*
 * implement the mvprintw commands.  Due to the variable number of
 * arguments, they cannot be macros.  Sigh....
 *
 */
int
mvprintw(y, x, fmt, args)
        reg int		y, x;
        char		*fmt;
        int		args;
{
	return move(y, x) == OK ? _sprintw(stdscr, fmt, &args) : ERR;
}

int
mvwprintw(win, y, x, fmt, args)
        reg WINDOW	*win;
        reg int		y, x;
        char		*fmt;
        int		args;
{
	return wmove(win, y, x) == OK ? _sprintw(win, fmt, &args) : ERR;
}
