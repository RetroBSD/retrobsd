/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <signal.h>
#include "curses.ext"

/*
 * handle stop and start signals
 */
void tstp(sig)
{
	SGTTY	tty;
	sigset_t oset, set;
#ifdef DEBUG
	if (outf)
		fflush(outf);
#endif
	/*
	 * Block window change and timer signals.  The latter is because
	 * applications use timers to decide when to repaint the screen.
	 */
	(void)sigemptyset(&set);
	(void)sigaddset(&set, SIGALRM);
	(void)sigaddset(&set, SIGWINCH);
	(void)sigprocmask(SIG_BLOCK, &set, &oset);

	tty = _tty;
	mvcur(0, COLS - 1, LINES - 1, 0);
	endwin();
	fflush(stdout);

	/* Unblock SIGTSTP. */
	(void)sigemptyset(&set);
	(void)sigaddset(&set, SIGTSTP);
	(void)sigprocmask(SIG_UNBLOCK, &set, NULL);

	/* Stop ourselves. */
	signal(SIGTSTP, SIG_DFL);
	kill(0, SIGTSTP);

	/* Time passes ... */

	/* Reset the SIGTSTP handler. */
	signal(SIGTSTP, (sig_t)tstp);

	_tty = tty;
	ioctl(_tty_ch, TIOCSETP, &_tty);

	/* Repaint the screen. */
	wrefresh(curscr);

	/* Reset the signals. */
	(void)sigprocmask(SIG_SETMASK, &oset, NULL);
}
