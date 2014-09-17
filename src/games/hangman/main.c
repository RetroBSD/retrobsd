#include <stdlib.h>
#include "hangman.h"

/*
 * die:
 *	Die properly.
 */
void die(int sig)
{
	mvcur(0, COLS - 1, LINES - 1, 0);
	endwin();
	putchar('\n');
	exit(0);
}

/*
 * This game written by Ken Arnold.
 */
int main()
{
	initscr();
	signal(SIGINT, die);
	setup();
	for (;;) {
		Wordnum++;
		playgame();
		Average = (Average * (Wordnum - 1) + Errors) / Wordnum;
	}
	/* NOTREACHED */
}
