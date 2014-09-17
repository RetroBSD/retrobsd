/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*************************************************************************
 *
 *	MOVE LIBRARY
 *
 *	This set of subroutines moves a cursor to a predefined
 *	location, independent of the terminal type.  If the
 *	terminal has an addressable cursor, it uses it.  If
 *	not, it optimizes for tabs (currently) even if you don't
 *      have them.
 *
 *	At all times the current address of the cursor must be maintained,
 *	and that is available as structure cursor.
 *
 *	The following calls are allowed:
 *		move(sp)	move to point sp.
 *		up()		move up one line.
 *		down()		move down one line.
 *		bs()		move left one space (except column 0).
 *		nd()		move right one space(no write).
 *		clear()		clear screen.
 *		home()		home.
 *		ll()		move to lower left corner of screen.
 *		cr()		carriage return (no line feed).
 *		print()		just like standard printf, but keeps track
 *				of cursor position. (Uses pstring).
 *		aprint()	same as print, but first argument is &point.
 *				(Uses pstring).
 *		pstring(s)	output the string of printing characters.
 *				However, '\r' is interpreted to mean return
 *				to column of origination AND do linefeed.
 *				'\n' causes <cr><lf>.
 *		putpad(str)	calls tputs to output character with proper
 *					padding.
 *		outch()		the output routine for a character used by
 *					tputs. It just calls putchar.
 *		pch(ch)		output character to screen and update
 *					cursor address (must be a standard
 *					printing character). WILL SCROLL.
 *		pchar(ps,ch)	prints one character if it is on the
 *					screen at the specified location;
 *					otherwise, dumps it.(no wrap-around).
 *
 *		getcap()	initializes strings for later calls.
 *		cap(string)	outputs the string designated in the termcap
 *					data base. (Should not move the cursor.)
 *		done(int)	returns the terminal to intial state.  If int
 *					is not 0, it exits.
 *
 *		same(&p1,&p2)	returns 1 if p1 and p2 are the same point.
 *		point(&p,x,y)	return point set to x,y.
 *
 *		delay(t)	causes an approximately constant delay
 *					independent of baudrate.
 *					Duration is ~ t/20 seconds.
 *
 ******************************************************************************/
#include "snake.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <term.h>
#include <unistd.h>

int CMlength;
int NDlength;
int BSlength;
int delaystr[10];

static char str[80];

int
outch(c)
{
	return putchar(c);
}

void
putpad(str)
char *str;
{
	if (str)
		tputs(str, 1, outch);
}

void
bs()
{
	if (cursor.col > 0) {
		putpad(BS);
		cursor.col--;
	}
}

void
nd()
{
	putpad(ND);
	cursor.col++;
	if (cursor.col == COLUMNS+1) {
		cursor.line++;
		cursor.col = 0;
		if (cursor.line >= LINES)cursor.line=LINES-1;
	}
}

void
right(sp)
struct point *sp;
{
	if (sp->col < cursor.col)
		print("ERROR:right() can't move left\n");
	while (sp->col > cursor.col) {
		nd();
	}
}

void
cr()
{
	outch('\r');
	cursor.col = 0;
}

void
ll()
{
	int l;
	struct point z;

	l = lcnt + 2;
	if (LL != NULL && LINES==l) {
	 	putpad(LL);
		cursor.line = LINES-1;
		cursor.col = 0;
		return;
	}
	z.col = 0;
	z.line = l-1;
	move(&z);
}

void
up()
{
	putpad(UP);
	cursor.line--;
}

void
down()
{
	putpad(DO);
	cursor.line++;
	if (cursor.line >= LINES)cursor.line=LINES-1;
}

void
gto(sp)
struct point *sp;
{
	int distance, f;

	if (cursor.line > LINES || cursor.line <0 ||
	    cursor.col <0 || cursor.col > COLUMNS)
		print("ERROR: cursor is at %d,%d\n",
			cursor.line,cursor.col);
	if (sp->line > LINES || sp->line <0 ||
	    sp->col <0 || sp->col >  COLUMNS)
		print("ERROR: target is %d,%d\n",sp->line,sp->col);
	if (sp->line == cursor.line) {
		if (sp->col > cursor.col)
                        right(sp);
		else{
			distance = (cursor.col -sp->col)*BSlength;
			if (cursor.col*NDlength < distance) {
				cr();
				right(sp);
			} else {
				while(cursor.col > sp->col)
                                        bs();
			}
		}
		return;
	}
				/*must change row */
	if (cursor.col - sp->col > (cursor.col >> 3)) {
		if (cursor.col == 0)f = 0;
		else f = -1;
	}
	else f = cursor.col >> 3;
	if (((sp->line << 1) + 1 < cursor.line - f) && (HO != 0)) {
			/*
			 * home quicker than rlf:
			 * (sp->line + f > cursor.line - sp->line)
			 */
		putpad(HO);
		cursor.col = cursor.line = 0;
		gto(sp);
		return;
	}
	if (((sp->line << 1) > cursor.line + LINES+1 + f) && (LL != 0)) {
		/* home,rlf quicker than lf
		 * (LINES+1 - sp->line + f < sp->line - cursor.line)
		 */
		if (cursor.line > f + 1) {
		/* is home faster than wraparound lf?
		 * (cursor.line + 20 - sp->line > 21 - sp->line + f)
		 */
			ll();
			gto(sp);
			return;
		}
	}
	if ((LL != 0) && (sp->line > cursor.line + (LINES >> 1) - 1))
		cursor.line += LINES;
	while(sp->line > cursor.line)down();
	while(sp->line < cursor.line)up();
	gto(sp);		/*can recurse since cursor.line = sp->line */
}

void
home()
{
	struct point z;

	if (HO != 0) {
		putpad(HO);
		cursor.col = cursor.line = 0;
		return;
	}
	z.col = z.line = 0;
	move(&z);
}

void
clear()
{
	int i;

	if (CL) {
		putpad(CL);
		cursor.col=cursor.line=0;
	} else {
		for(i=0; i<LINES; i++) {
			putchar('\n');
		}
		cursor.line = LINES - 1;
		home();
	}
}

void
move(sp)
struct point *sp;
{
	struct point z;

	if (sp->line <0 || sp->col <0 || sp->col > COLUMNS) {
		print("move to [%d,%d]?",sp->line,sp->col);
		return;
	}
	if (sp->line >= LINES) {
		move(point(&z,sp->col,LINES-1));
		while(sp->line-- >= LINES)putchar('\n');
		return;
	}
	if (sp->line == cursor.line) {
                if (sp->col == cursor.col)
                        return;
                if (sp->col == cursor.col-1) {
                        bs();
                        return;
                }
        }
	if (sp->line == cursor.line+1) {
                if (sp->col == cursor.col) {
                        down();
                        return;
                }
                if (sp->col == cursor.col-1) {
                        down();
                        bs();
                        return;
                }
        }
        char *cmstr = tgoto(CM, sp->col, sp->line);
        putpad(cmstr);
        cursor.line = sp->line;
        cursor.col = sp->col;
}

void
pch(c)
{
	outch(c);
	if (++cursor.col >= COLUMNS) {
		cursor.col = -1;
		cursor.line = -1;
	}
}

void
pstring(s)
char *s;
{
	struct point z;
	int stcol;

	stcol = cursor.col;
	while (s[0] != '\0') {
		switch (s[0]) {
		case '\n':
			move(point(&z,0,cursor.line+1));
			break;
		case '\r':
			move(point(&z,stcol,cursor.line+1));
			break;
		case '\t':
			z.col = (((cursor.col + 8) >> 3) << 3);
			z.line = cursor.line;
			move(&z);
			break;
		case '\b':
			bs();
			break;
		case CTRL('g'):
			outch(CTRL('g'));
			break;
		default:
			if (s[0] < ' ')
                                break;
			pch(s[0]);
		}
		s++;
	}
}

void
aprint(struct point *ps, char *fmt, ...)
{
        va_list ap;
	struct point p;

	p.line = ps->line+1;
        p.col = ps->col+1;
	move(&p);
        va_start(ap, fmt);
        vsprintf(str, fmt, ap);
        va_end(ap);
	pstring(str);
}

void
print(char *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
        vsprintf(str, fmt, ap);
        va_end(ap);
	pstring(str);
}

void
pchar(ps,ch)
struct point *ps;
char ch;
{
	struct point p;

	p.col = ps->col + 1;
        p.line = ps->line + 1;
	if (p.col < 0 || p.line < 0)
	        return;
	if ((p.line < LINES && p.col < COLUMNS) ||
	    (p.col == COLUMNS && p.line < LINES-1)) {
		move(&p);
		pch(ch);
	}
}

void
delay(t)
int t;
{
        usleep(t * 50);
}

void
cook()
{
	delay(1);
	putpad(TE);
	putpad(KE);
	fflush(stdout);
#ifdef CROSS
        ioctl(0, TCSETAW, &origtty);
#else
        ioctl(0, TIOCSETP, &origtty);
#endif
#ifdef TIOCSLTC
	ioctl(0, TIOCSLTC, &olttyc);
#endif
}

void
done()
{
	cook();
	exit(0);
}

void
raw()
{
#ifdef CROSS
        ioctl(0, TCSETAW, &newtty);
#else
        ioctl(0, TIOCSETP, &newtty);
#endif
#ifdef TIOCSLTC
	ioctl(0, TIOCSLTC, &nlttyc);
#endif
}

int
same(sp1,sp2)
struct point *sp1, *sp2;
{
	if ((sp1->line == sp2->line) && (sp1->col == sp2->col))return(1);
	return(0);
}

struct point *
point(ps,x,y)
struct point *ps;
int x,y;
{
	ps->col=x;
	ps->line=y;
	return(ps);
}

char *ap;

void
getcap()
{
	char *term;
	char *xPC;

	term = getenv("TERM");
	if (term==0) {
		fprintf(stderr, "No TERM in environment\n");
		exit(1);
	}

	switch (tgetent(tbuf, term)) {
	case -1:
		fprintf(stderr, "Cannot open termcap file\n");
		exit(2);
	case 0:
		fprintf(stderr, "%s: unknown terminal", term);
		exit(3);
	}

	ap = tcapbuf;

	LINES = tgetnum("li");
	COLUMNS = tgetnum("co");
	lcnt = LINES;
	ccnt = COLUMNS - 1;

	BW = tgetflag("bw");

	ND = tgetstr("nd", &ap);
	UP = tgetstr("up", &ap);

	DO = tgetstr("do", &ap);
	if (DO == 0)
		DO = "\n";

	BS = tgetstr("bc", &ap);
	if (BS == 0 && tgetflag("bs"))
		BS = "\b";
	if (BS)
		xBC = *BS;

	HO = tgetstr("ho", &ap);
	CL = tgetstr("cl", &ap);
	CM = tgetstr("cm", &ap);
	LL = tgetstr("ll", &ap);

	KL = tgetstr("kl", &ap);
	KR = tgetstr("kr", &ap);
	KU = tgetstr("ku", &ap);
	KD = tgetstr("kd", &ap);
	Klength = strlen(KL);
		/*	NOTE:   If KL, KR, KU, and KD are not
		 *		all the same length, some problems
		 *		may arise, since tests are made on
		 *		all of them together.
		 */

	TI = tgetstr("ti", &ap);
	TE = tgetstr("te", &ap);
	KS = tgetstr("ks", &ap);
	KE = tgetstr("ke", &ap);

	xPC = tgetstr("pc", &ap);
	if (xPC)
		PC = *xPC;

	NDlength = strlen(ND);
	BSlength = strlen(BS);
	if (CM == 0) {
		fprintf(stderr, "Terminal must have addressible cursor\n");
		exit(5);
	}
	if (tgetflag("os")) {
		fprintf(stderr, "Terminal must not overstrike\n");
		exit(5);
	}
	if (LINES <= 0 || COLUMNS <= 0) {
		fprintf(stderr, "Must know the screen size\n");
		exit(5);
	}

#ifdef CROSS
        ioctl(0, TCGETA, &origtty);
	newtty = origtty;
        newtty.c_lflag &= ~(ICANON | ECHO);
        newtty.c_oflag &= ~ONLCR;
        newtty.c_cc[VMIN] = 1;
        newtty.c_cc[VTIME] = 0;
#else
        ioctl(0, TIOCGETP, &origtty);
	newtty = origtty;
	newtty.sg_flags &= ~(ECHO|CRMOD|XTABS);
	newtty.sg_flags |= CBREAK;
#endif
#ifdef TIOCGLTC
	ioctl(0, TIOCGLTC, &olttyc);
	nlttyc = olttyc;
	nlttyc.t_suspc = '\377';
	nlttyc.t_dsuspc = '\377';
#endif
	raw();
	signal(SIGINT, stop);

	putpad(KS);
	putpad(TI);
	point(&cursor,0,LINES-1);
}
