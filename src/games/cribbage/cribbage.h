/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)cribbage.h	5.1 (Berkeley) 5/30/85
 */
# include	<curses.h>

extern  CARD		deck[ CARDS ];		/* a deck */
extern  CARD		phand[ FULLHAND ];	/* player's hand */
extern  CARD		chand[ FULLHAND ];	/* computer's hand */
extern  CARD		crib[ CINHAND ];	/* the crib */
extern  CARD		turnover;		/* the starter */

extern  CARD		known[ CARDS ];		/* cards we have seen */
extern  int		knownum;		/* # of cards we know */

extern  int		pscore;			/* player's score */
extern  int		cscore;			/* comp's score */
extern  int		glimit;			/* points to win game */

extern  int		pgames;			/* player's games won */
extern  int		cgames;			/* comp's games won */
extern  int		gamecount;		/* # games played */
extern	int		Lastscore[2];		/* previous score for each */

extern  BOOLEAN		iwon;			/* if comp won last */
extern  BOOLEAN		explain;		/* player mistakes explained */
extern  BOOLEAN		rflag;			/* if all cuts random */
extern  BOOLEAN		quiet;			/* if suppress random mess */
extern	BOOLEAN		playing;		/* currently playing game */

extern  char		explstr[];		/* string for explanation */

char *getlin(void);
void bye(int sig);
void msg(char *fmt, ...);
void addmsg(char *fmt, ...);
BOOLEAN msgcard(CARD c, BOOLEAN brief);
void endmsg(void);
int getuchar(void);
void makedeck(CARD d[]);
void shuffle(CARD d[]);
void sorthand(CARD h[], int n);
void makeknown(CARD h[], int n);
void prhand(CARD h[], int n, WINDOW *win, BOOLEAN blank);
void prcard(WINDOW *win, int y, int x, CARD c, BOOLEAN blank);
void cdiscard(BOOLEAN mycrib);
int infrom(CARD hand[], int n, char *prompt);
void cremove(CARD a, CARD d[], int n);
BOOLEAN chkscr(int *scr, int inc);
BOOLEAN anymove(CARD hand[], int n, int sum);
int scorehand(CARD hand[], CARD starter, int n, BOOLEAN crb, BOOLEAN do_explain);
int number(int lo, int hi, char *prompt);
void do_wait(void);
int adjust(CARD cb[], CARD tnv);
int pegscore(CARD crd, CARD tbl[], int n, int sum);
int cchose(CARD h[], int n, int s);
BOOLEAN plyrhand(CARD hand[], char *s);
BOOLEAN comphand(CARD h[], char *s);
BOOLEAN isone(CARD a, CARD b[], int n);
