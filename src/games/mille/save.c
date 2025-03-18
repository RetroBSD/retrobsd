#include "mille.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef	unctrl
#   include "unctrl.h"
#endif

#ifdef	attron
#   include <term.h>
#   define _tty	cur_term->Nttyb
#endif

/*
 * @(#)save.c	1.3 (2.11BSD) 1996/3/21
 */

typedef	struct stat	STAT;

/*
 *	This routine saves the current game for use at a later date
 */
int
save() {

	char	*sp;
	int	outf;
	time_t	*tp;
	char	buf[80];
	time_t	tme;
	STAT	junk;

	tp = &tme;
	if (Fromfile && getyn(SAMEFILEPROMPT)) {
		strcpy(buf, Fromfile);
		sp = buf;
	} else {
over:
		prompt(FILEPROMPT);
		leaveok(Board, FALSE);
		refresh();
		sp = buf;
		while ((*sp = readch()) != '\n') {
			if (*sp == killchar())
				goto over;
			else if (*sp == erasechar()) {
				if (--sp < buf)
					sp = buf;
				else {
					addch('\b');
					/*
					 * if the previous char was a control
					 * char, cover up two characters.
					 */
					if (*sp < ' ')
						addch('\b');
					clrtoeol();
				}
			}
			else
				addstr(unctrl(*sp++));
			refresh();
		}
		*sp = '\0';
		leaveok(Board, TRUE);
	}

	/*
	 * check for existing files, and confirm overwrite if needed
	 */

	if (sp == buf || (!Fromfile && stat(buf, &junk) > -1
	    && getyn(OVERWRITEFILEPROMPT) == FALSE))
		return FALSE;

	if ((outf = creat(buf, 0644)) < 0) {
		error(strerror(errno), 0);
		return FALSE;
	}
	mvwaddstr(Score, ERR_Y, ERR_X, buf);
	wrefresh(Score);
	time(tp);			/* get current time		*/
	strcpy(buf, ctime(tp));
	for (sp = buf; *sp != '\n'; sp++)
		continue;
	*sp = '\0';
	varpush(outf, write);
	close(outf);
	wprintw(Score, " [%s]", buf);
	wclrtoeol(Score);
	wrefresh(Score);
	return TRUE;
}

/*
 *	This does the actual restoring.  It returns TRUE if the
 * backup was made on exiting, in which case certain things must
 * be cleaned up before the game starts.
 */
int
rest_f(
char	*file) {

	char	*sp;
	int	inf;
	char	buf[80];
	STAT	sbuf;

	if ((inf = open(file, 0)) < 0) {
		perror(file);
		exit(1);
	}
	if (fstat(inf, &sbuf) < 0) {		/* get file stats	*/
		perror(file);
		exit(1);
	}
	varpush(inf, read);
	close(inf);
	strcpy(buf, ctime(&sbuf.st_mtime));
	for (sp = buf; *sp != '\n'; sp++)
		continue;
	*sp = '\0';
	/*
	 * initialize some necessary values
	 */
	sprintf(Initstr, "%s [%s]\n", file, buf);
	Fromfile = file;
	return !On_exit;
}
