#include "mille.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef attron
#   include <term.h>
#endif

/*
 * @(#)mille.c	1.3.1 (2.11BSD GTE) 1/16/95
 */

char	_sobuf[BUFSIZ];

/*
 *	Routine to trap rubouts, and make sure they really want to
 * quit.
 */
void
rub(int sig) {

	signal(SIGINT, SIG_IGN);
	if (getyn(REALLYPROMPT))
		die();
	signal(SIGINT, rub);
}

int
main(
int	ac,
char	*av[]) {

	bool	restore;
	unsigned avs[3];

	if (strcmp(av[0], "a.out") == 0) {
		outf = fopen("q", "w");
		setbuf(outf, NULL);
		Debug = TRUE;
	}
	restore = FALSE;

	if (getloadavg(avs, 3) >= 0 && avs[2] > 400) {
		printf("Sorry.  The load average is too high.\n");
		printf("Please try again later\n");
		exit(1);
	}

	switch (ac) {
	  case 2:
		rest_f(av[1]);
		restore = TRUE;
	  case 1:
		break;
	  default:
		printf("usage: milles [ restore_file ]\n");
		exit(-1);
		/* NOTREACHED */
	}
	setbuf(stdout, _sobuf);
	Play = PLAYER;
	initscr();
# ifdef attron
#	define	CA	cursor_address
# endif
	if (!CA) {
		printf("Sorry.  Need cursor addressing to play mille\n");
		exit(-1);
	}
	delwin(stdscr);
	stdscr = Board = newwin(BOARD_Y, BOARD_X, 0, 0);
	Score = newwin(SCORE_Y, SCORE_X, 0, 40);
	Miles = newwin(MILES_Y, MILES_X, 17, 0);
#ifdef attron
	idlok(Board, TRUE);
	idlok(Score, TRUE);
	idlok(Miles, TRUE);
#endif
	leaveok(Score, TRUE);
	leaveok(Miles, TRUE);
	clearok(curscr, TRUE);
# ifndef PROF
	srandom(getpid());
# else
	srandom(0);
# endif
	crmode();
	noecho();
	signal(SIGINT, rub);
	for (;;) {
		if (!restore || (Player[PLAYER].total >= 5000
		    || Player[COMP].total >= 5000)) {
			if (Player[COMP].total < Player[PLAYER].total)
				Player[PLAYER].games++;
			else if (Player[COMP].total > Player[PLAYER].total)
				Player[COMP].games++;
			Player[COMP].total = 0;
			Player[PLAYER].total = 0;
		}
		do {
			if (!restore)
				Handstart = Play = other(Handstart);
			if (!restore || On_exit) {
				shuffle();
				init();
			}
			newboard();
			if (restore)
				mvwaddstr(Score, ERR_Y, ERR_X, Initstr);
			prboard();
			do {
				domove();
				if (Finished)
					newscore();
				prboard();
			} while (!Finished);
			check_more();
			restore = On_exit = FALSE;
		} while (Player[COMP].total < 5000
		    && Player[PLAYER].total < 5000);
	}
}

/*
 *	Time to go beddy-by
 */
void
die() {

	signal(SIGINT, SIG_IGN);
	if (outf)
		fflush(outf);
	mvcur(0, COLS - 1, LINES - 1, 0);
	endwin();
	exit(1);
}
