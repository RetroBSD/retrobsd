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
