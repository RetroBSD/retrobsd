/*
 * Worm.  Written by Michael Toy
 * UCSC
 *
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifdef CROSS
#   define FILE int                     /* quick hack for macosx */
#   include </usr/include/curses.h>
#   include </usr/include/ctype.h>
#   undef FILE
#else
#   include <curses.h>
#   include <ctype.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define newlink()       (struct body *) malloc(sizeof (struct body));
#define HEAD            '@'
#define BODY            'o'
#define LENGTH          7
#define RUNLEN          8
#define CNTRL(p)        (p & 037)

WINDOW *tv;
WINDOW *stw;

struct body {
	int x;
	int y;
	struct body *prev;
	struct body *next;
} *head, *tail, goody;

int growing = 0;
int running = 0;
int score = 0;
int start_len = LENGTH;
char lastch;

extern int printf(const char *, ...);

void display(pos, chr)
        struct body *pos;
        char chr;
{
	wmove(tv, pos->y, pos->x);
	waddch(tv, chr);
}

void life()
{
	register struct body *bp, *np = 0;
	register int i;

	head = newlink();
	head->x = start_len+2;
	head->y = 12;
	head->next = NULL;
	display(head, HEAD);
	for (i = 0, bp = head; i < start_len; i++, bp = np) {
		np = newlink();
		np->next = bp;
		bp->prev = np;
		np->x = bp->x - 1;
		np->y = bp->y;
		display(np, BODY);
	}
	tail = np;
	tail->prev = NULL;
}

void leave()
{
	endwin();
	exit(0);
}

void setup()
{
	clear();
	refresh();
	touchwin(stw);
	wrefresh(stw);
	touchwin(tv);
	wrefresh(tv);
	alarm(1);
}

void suspend()
{
	move(LINES-1, 0);
	refresh();
	endwin();
#ifdef SIGTSTP
	kill(getpid(), SIGTSTP);
	signal(SIGTSTP, suspend);
#else
        {
	char *sh = getenv("SHELL");
	if (sh == NULL)
		sh = "/bin/sh";
	system(sh);
	}
#endif
	crmode();
	noecho();
	setup();
}

void crash()
{
	sleep(1);
	clear();
	move(23, 0);
	refresh();
	printf("Well you ran into something and the game is over.\r\n");
	printf("Your final score was %d\r\n", score);
	leave();
}

int rnd(range)
{
	return abs((rand()>>5)+(rand()>>5)) % range;
}

void newpos(bp)
        struct body * bp;
{
	do {
		bp->y = rnd(LINES-3)+ 2;
		bp->x = rnd(COLS-3) + 1;
		wmove(tv, bp->y, bp->x);
	} while (winch(tv) != ' ');
}

void prize()
{
	int value;

	value = rnd(9) + 1;
	newpos(&goody);
	waddch(tv, value+'0');
	wrefresh(tv);
}

void process(ch)
        char ch;
{
	register int x,y;
	struct body *nh;

	alarm(0);
	x = head->x;
	y = head->y;
	switch (ch) {
        case 'h': x--; break;
        case 'j': y++; break;
        case 'k': y--; break;
        case 'l': x++; break;
        case 'H': x--; running = RUNLEN; ch = tolower(ch); break;
        case 'J': y++; running = RUNLEN/2; ch = tolower(ch); break;
        case 'K': y--; running = RUNLEN/2; ch = tolower(ch); break;
        case 'L': x++; running = RUNLEN; ch = tolower(ch); break;
        case '\f': setup(); return;
        case CNTRL('Z'): suspend(); return;
        case CNTRL('C'): crash(); return;
        case CNTRL('D'): crash(); return;
        default: if (! running) alarm(1); return;
	}
	lastch = ch;
	if (growing == 0) {
		display(tail, ' ');
		tail->next->prev = NULL;
		nh = tail->next;
		free(tail);
		tail = nh;
	} else
                growing--;
	display(head, BODY);
	wmove(tv, y, x);
	if (isdigit(ch = winch(tv))) {
		growing += ch-'0';
		prize();
		score += growing;
		running = 0;
		wmove(stw, 0, 68);
		wprintw(stw, "Score: %3d", score);
		wrefresh(stw);
	} else if (ch != ' ')
                crash();
	nh = newlink();
	nh->next = NULL;
	nh->prev = head;
	head->next = nh;
	nh->y = y;
	nh->x = x;
	display(nh, HEAD);
	head = nh;
	if (! running)
		wrefresh(tv);
	if (! running)
		alarm(1);
}

void wake()
{
	signal(SIGALRM, wake);
	process(lastch);
}

int main(argc, argv)
        char **argv;
{
	char ch;

	if (argc == 2)
		start_len = atoi(argv[1]);
	if ((start_len <= 0) || (start_len > 500))
		start_len = LENGTH;
	srand(getpid());
	signal(SIGALRM, wake);
	signal(SIGINT, leave);
	signal(SIGQUIT, leave);
#ifdef SIGTSTP
	signal(SIGTSTP, suspend);	/* process control signal */
#endif
	initscr();
	crmode();
	noecho();
	clear();
	stw = newwin(1, COLS-1, 0, 0);
	tv = newwin(LINES-1, COLS-1, 1, 0);
	box(tv, '*', '*');
	scrollok(tv, FALSE);
	scrollok(stw, FALSE);
	wmove(stw, 0, 0);
	wprintw(stw, " Worm");
	refresh();
	wrefresh(stw);
	wrefresh(tv);
	life();			/* Create the worm */
	prize();		/* Put up a goal */
	while (1) {
		if (running) {
			running--;
			process(lastch);
		} else {
                        if (read(0, &ch, 1) >= 0)
                                process(ch);
		}
	}
}
