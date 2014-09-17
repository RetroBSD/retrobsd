/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "robots.h"
#include <signal.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef CROSS
static int
fputchar(int c)
{
    return putchar(c);
}
#endif

/*
 * quit:
 *	Leave the program elegantly.
 */
void
quit(int sig)
{
	mvcur(0, COLS - 1, LINES - 1, 0);
#ifndef CROSS
	if (CE) {
		tputs(CE, 1, fputchar);
		endwin();
	} else
#endif
        {
		endwin();
		putchar('\n');
	}
	exit(0);
	/* NOTREACHED */
}

/*
 * another:
 *	See if another game is desired
 */
int
another()
{
	register int	y;

#ifdef	FANCY
	if ((Stand_still || Pattern_roll) && !Newscore)
		return TRUE;
#endif

	if (query("Another game?")) {
		if (Full_clear) {
			for (y = 1; y <= Num_scores; y++) {
				move(y, 1);
				clrtoeol();
			}
			refresh();
		}
		return TRUE;
	}
	return FALSE;
}

int
main(ac, av)
int	ac;
char	**av;
{
	register char	*sp;
	register bool	bad_arg;
	register bool	show_only;
	extern char	*Scorefile;
	extern int	Max_per_uid;
	extern char	*rindex();

	show_only = FALSE;
	if (ac > 1) {
		bad_arg = FALSE;
		for (++av; ac > 1 && *av[0]; av++, ac--)
			if (av[0][0] != '-')
				if (isdigit(av[0][0]))
					Max_per_uid = atoi(av[0]);
				else {
					if (setuid(getuid()) < 0)
					    /*ignore*/;
					if (setgid(getgid()) < 0)
					    /*ignore*/;
					Scorefile = av[0];
#ifdef	FANCY
					sp = rindex(Scorefile, '/');
					if (sp == NULL)
						sp = Scorefile;
					if (strcmp(sp, "pattern_roll") == 0)
						Pattern_roll = TRUE;
					else if (strcmp(sp, "stand_still") == 0)
						Stand_still = TRUE;
					if (Pattern_roll || Stand_still)
						Teleport = TRUE;
#endif
				}
			else
				for (sp = &av[0][1]; *sp; sp++)
					switch (*sp) {
					  case 's':
						show_only = TRUE;
						break;
					  case 'r':
						Real_time = TRUE;
						break;
					  case 'a':
						Start_level = 4;
						break;
					  case 'j':
						Jump = TRUE;
						break;
					  case 't':
						Teleport = TRUE;
						break;
					  default:
						fprintf(stderr, "robots: uknown option: %c\n", *sp);
						bad_arg = TRUE;
						break;
					}
		if (bad_arg) {
			exit(1);
			/* NOTREACHED */
		}
	}

	if (show_only) {
		show_score();
		exit(0);
		/* NOTREACHED */
	}

	initscr();
	signal(SIGINT, quit);
	crmode();
	noecho();
	nonl();
	if (LINES != Y_SIZE || COLS != X_SIZE) {
		if (LINES < Y_SIZE || COLS < X_SIZE) {
			endwin();
			printf("Need at least a %dx%d screen\n", Y_SIZE, X_SIZE);
			exit(1);
		}
		delwin(stdscr);
		stdscr = newwin(Y_SIZE, X_SIZE, 0, 0);
	}

	srand(getpid());
	if (Real_time)
		signal(SIGALRM, move_robots);
	do {
		init_field();
		for (Level = Start_level; !Dead; Level++) {
			make_level();
			play_level();
		}
		move(My_pos.y, My_pos.x);
		printw("AARRrrgghhhh....");
		refresh();
		score();
	} while (another());
	quit(0);
	return 0;
}
