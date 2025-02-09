/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "back.h"
#include <signal.h>
#include <unistd.h>

extern const char	*hello[];
extern const char	*list[];
extern const char	*intro1[];
extern const char	*intro2[];
extern const char	*moves[];
extern const char	*remove[];
extern const char	*hits[];
extern const char	*endgame[];
extern const char	*doubl[];
extern const char	*stragy[];
extern const char	*prog[];
extern const char	*lastch[];

char args[100];		/* args passed to teachgammon and back */
int board[26];		/* board:  negative values are white, positive are red */
int dice[2];		/* value of dice */
int mvlim;		/* 'move limit':  max. number of moves */
int mvl;		/* working copy of mvlim */
int p[5];		/* starting position of moves */
int g[5];		/* ending position of moves (goals) */
int h[4];		/* flag for each move if a man was hit */
int cturn;		/* whose turn it currently is */
int d0;			/* flag if dice have been reversed from original position */
int table[6][6];	/* odds table for possible rolls */
int rscore;		/* red's score */
int wscore;		/* white's score */
int gvalue;		/* value of game (64 max.) */
int dlast;		/* who doubled last (0 = neither) */
int bar;		/* position of bar for current player */
int home;		/* position of home for current player */
int off[2];		/* number of men off board */
int *offptr;		/* pointer to off for current player */
int *offopp;		/* pointer to off for opponent */
int in[2];		/* number of men in inner table */
int *inptr;		/* pointer to in for current player */
int *inopp;		/* pointer to in for opponent */

int ncin;		/* number of characters in cin */
char cin[100];		/* input line of current move */

char **colorptr;	/* color of current player */
char **Colorptr;	/* color of current player, capitalized */
int colen;		/* length of color of current player */

struct sgttyb tty;	/* tty information buffer */
int old;		/* original tty status */
int noech;		/* original tty status without echo */
int raw;		/* raw tty status, no echo */

int curr;		/* row position of cursor */
int curc;		/* column position of cursor */
int begscr;		/* 'beginning' of screen */
char ospeed;		/* tty output speed */

const char *helpm[] = {
	"\nEnter a space or newline to roll, or",
	"     b   to display the board",
	"     d   to double",
	"     q   to quit\n",
	0
};

const char *contin[] = {
	"",
	0
};

int
main (argc,argv)
        int	argc;
        char	**argv;
{
	register int	i;

	signal (2,getout);
	if (ioctl (0, TIOCGETP, &tty) == -1)		/* get old tty mode */
		errexit ("teachgammon(gtty)");
	old = tty.sg_flags;
#ifdef V7
	raw = ((noech = old & ~ECHO) | CBREAK);		/* set up modes */
#else
	raw = ((noech = old & ~ECHO) | RAW);		/* set up modes */
#endif
	tflag = getcaps (getenv ("TERM"));
	while (*++argv != 0)
		getarg (&argv);
	if (tflag)  {
		noech &= ~(CRMOD|XTABS);
		raw &= ~(CRMOD|XTABS);
		clear();
	}
	text (hello);
	text (list);
	i = text (contin);
	if (i == 0)
		i = 2;
	init();
	while (i)
		switch (i)  {
		case 1:
			leave();
		case 2:
			i = text(intro1);
			if (i)
				break;
			wrboard();
			i = text(intro2);
			if (i)
				break;
		case 3:
			i = text(moves);
			if (i)
				break;
		case 4:
			i = text(remove);
			if (i)
				break;
		case 5:
			i = text(hits);
			if (i)
				break;
		case 6:
			i = text(endgame);
			if (i)
				break;
		case 7:
			i = text(doubl);
			if (i)
				break;
		case 8:
			i = text(stragy);
			if (i)
				break;
		case 9:
			i = text(prog);
			if (i)
				break;
		case 10:
			i = text(lastch);
			if (i)
				break;
		}
	tutor();
        return 0;
}

void
leave()
{
	if (tflag)
		clear();
	else
		writec ('\n');
	fixtty(old);
	execl (EXEC, "backgammon", args, "n", (char*)0);
	writel ("Help! Backgammon program is missing\007!!\n");
	exit (-1);
}
