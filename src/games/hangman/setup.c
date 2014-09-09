#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "hangman.h"

/*
 * setup:
 *	Set up the strings on the screen.
 */
void
setup()
{
	register char		**sp;
	static struct stat	sbuf;

	noecho();
	crmode();

	mvaddstr(PROMPTY, PROMPTX, "Guess:");
	mvaddstr(GUESSY, GUESSX, "Guessed:");
	mvaddstr(NUMBERY, NUMBERX, "Word #:");
	mvaddstr(AVGY, AVGX, "Current Average:");
	mvaddstr(AVGY + 1, AVGX, "Overall Average:");
	mvaddstr(KNOWNY, KNOWNX, "Word: ");

	for (sp = Noose_pict; *sp != NULL; sp++) {
		move(sp - Noose_pict, 0);
		addstr(*sp);
	}

	srandom(time(NULL) + getpid());
	if ((Dict = fopen(DICT, "r")) == NULL) {
		perror(DICT);
		endwin();
		exit(1);
	}
	fstat(fileno(Dict), &sbuf);
	Dict_size = sbuf.st_size;
}
