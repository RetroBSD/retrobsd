/* This file is in the public domain. */

/*
 * This file comes from mg1a.
 * Uses the panic function from OpenBSD's mg.
 */

/*
 *		Ultrix-32 and Unix terminal I/O.
 * The functions in this file
 * negotiate with the operating system for
 * keyboard characters, and write characters to
 * the display in a barely buffered fashion.
 */

#include <stdio.h>
#include <sgtty.h>
#include <stdlib.h>
#include <term.h>
#include <unistd.h>
#undef CTRL
#include "estruct.h"

void ttflush(void);
void panic(char *);

extern void getwinsize();

#define NROW	66			/* Rows.			*/
#define NCOL	132			/* Columns.			*/
#define	NOBUF	512			/* Output buffer size.		*/

char	obuf[NOBUF];			/* Output buffer.		*/
int	nobuf;
struct	sgttyb	oldtty;			/* V6/V7 stty data.		*/
struct	sgttyb	newtty;
struct	tchars	oldtchars;		/* V7 editing.			*/
struct	tchars	newtchars;
struct	ltchars oldltchars;		/* 4.2 BSD editing.		*/
struct	ltchars	newltchars;

/*
 * This function gets called once, to set up
 * the terminal channel.
 */
void ttopen(void) {
        register char *tv_stype;
        char *getenv(), *tgetstr(), tcbuf[1024];

	if (ioctl(0, TIOCGETP, (char *) &oldtty) < 0)
		panic("ttopen can't get sgtty");
	newtty.sg_ospeed = oldtty.sg_ospeed;
	newtty.sg_ispeed = oldtty.sg_ispeed;
	newtty.sg_erase  = oldtty.sg_erase;
	newtty.sg_kill   = oldtty.sg_kill;
	newtty.sg_flags  = oldtty.sg_flags;
	newtty.sg_flags &= ~(ECHO|CRMOD);	/* Kill echo, CR=>NL.	*/
	newtty.sg_flags |= RAW|ANYP;		/* raw mode for 8 bit path.*/
	if (ioctl(0, TIOCSETP, (char *) &newtty) < 0)
		panic("ttopen can't set sgtty");
	if (ioctl(0, TIOCGETC, (char *) &oldtchars) < 0)
		panic("ttopen can't get chars");
	newtchars.t_intrc  = 0xFF;		/* Interrupt.		*/
	newtchars.t_quitc  = 0xFF;		/* Quit.		*/
	newtchars.t_startc = 0xFF;		/* ^Q, for terminal.	*/
	newtchars.t_stopc  = 0xFF;		/* ^S, for terminal.	*/
	newtchars.t_eofc   = 0xFF;
	newtchars.t_brkc   = 0xFF;
	if (ioctl(0, TIOCSETC, (char *) &newtchars) < 0)
		panic("ttopen can't set chars");
	if (ioctl(0, TIOCGLTC, (char *) &oldltchars) < 0)
		panic("ttopen can't get ltchars");
	newltchars.t_suspc  = 0xFF;		/* Suspend #1.		*/
	newltchars.t_dsuspc = 0xFF;		/* Suspend #2.		*/
	newltchars.t_rprntc = 0xFF;
	newltchars.t_flushc = 0xFF;		/* Output flush.	*/
	newltchars.t_werasc = 0xFF;
	newltchars.t_lnextc = 0xFF;		/* Literal next.	*/
	if (ioctl(0, TIOCSLTC, (char *) &newltchars) < 0)
		panic("ttopen can't set ltchars");

/* do this the REAL way */
        if ((tv_stype = getenv("TERM")) == NULL)
                panic("TERM not defined");

        if((tgetent(tcbuf, tv_stype)) != 1)
                panic("Unknown terminal type");

	getwinsize();
}

/*
 * This function gets called just
 * before we go back home to the shell. Put all of
 * the terminal parameters back.
 */
void ttclose(void) {
	ttflush();
	if (ioctl(0, TIOCSLTC, (char *) &oldltchars) < 0)
		panic("ttclose can't set ltchars");
	if (ioctl(0, TIOCSETC, (char *) &oldtchars) < 0)
		panic("ttclose can't set chars");
	if (ioctl(0, TIOCSETP, (char *) &oldtty) < 0)
		panic("ttclose can't set sgtty");
}

/*
 * Write character to the display.
 * Characters are buffered up, to make things
 * a little bit more efficient.
 */
int ttputc(int c) {
	if (nobuf >= NOBUF)
		ttflush();
	obuf[nobuf++] = c;
	return (c);
}

/*
 * Flush output.
 */
void ttflush(void) {
	if (nobuf != 0) {
		if (write(1, obuf, nobuf) != nobuf)
			panic("ttflush write failed");
		nobuf = 0;
	}
}

/*
 * Read character from terminal.
 * All 8 bits are returned, so that you can use
 * a multi-national terminal.
 */
int ttgetc(void) {
	char buf[1];

	while (read(0, &buf[0], 1) != 1);
	return (buf[0] & 0xFF);
}

/*
 * typeahead returns TRUE if there are characters available to be read
 * in.
 */
int typeahead(void) {
	int x;

	return((ioctl(0, FIONREAD, (char *) &x) < 0) ? 0 : x);
}

/*
 * panic - just exit, as quickly as we can.
 * From OpenBSD's mg.
 */
void panic(char *s)
{
    ttclose();
    (void) fputs("panic: ", stderr);
    (void) fputs(s, stderr);
    (void) fputc('\n', stderr);
    exit(1);
}
