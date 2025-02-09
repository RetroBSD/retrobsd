/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)snake.h	5.1 (Berkeley) 5/30/85
 */
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <math.h>
#ifdef CROSS
#   include <termios.h>
#   define sgttyb termio
#else
#   include <sgtty.h>
#endif

#define ESC	'\033'

extern char *CL, *UP, *DO, *ND, *BS,
	*HO, *CM, *LL,
	*KL, *KR, *KU, *KD,
	*TI, *TE, *KS, *KE;
extern int LINES, COLUMNS;  /* physical screen size. */
extern int lcnt, ccnt;      /* user's idea of screen size */
extern char xBC, PC;
extern int BW;
extern char tbuf[1024], tcapbuf[128];
extern int Klength;         /* length of KX strings */
extern int chunk;           /* amount of money given at a time */

#ifdef	debug
#   define cashvalue	(loot-penalty)/25
#else
#   define cashvalue	chunk*(loot-penalty)/25
#endif

struct point {
	int col, line;
};
extern struct point cursor;
extern struct sgttyb origtty, newtty;
#ifdef TIOCLGET
extern struct ltchars olttyc, nlttyc;
#endif

#undef CTRL
#define CTRL(x) (x - 'A' + 1)

struct point *point(struct point *ps, int x, int y);

void print(char *fmt, ...);
void aprint(struct point *ps, char *fmt, ...);
void move(struct point *sp);
void stop(int sig);
void pchar(struct point *ps, char ch);
void putpad(char *str);
void clear(void);
void delay(int t);
void cook(void);
void raw(void);
void ll(void);
void done(void);
void getcap(void);

int same(struct point *sp1, struct point *sp2);
