#include "mille.h"
#ifndef	unctrl
#   include "unctrl.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>

#ifdef	attron
#   include <term.h>
#   define _tty	cur_term->Nttyb
#endif

/*
 * @(#)misc.c	1.2 (Berkeley) 3/28/83
 */

#define	NUMSAFE	4

/* VARARGS1 */
int
error(str, arg)
char	*str;
{
	stdscr = Score;
	mvprintw(ERR_Y, ERR_X, str, arg);
	clrtoeol();
	putchar('');
	refresh();
	stdscr = Board;
	return FALSE;
}

CARD
getcard()
{
	int	c, c1;

	for (;;) {
		while ((c = readch()) == '\n' || c == '\r' || c == ' ')
			continue;
		if (islower(c))
			c = toupper(c);
		if (c == killchar() || c == erasechar())
			return -1;
		addstr(unctrl(c));
		clrtoeol();
		switch (c) {
		  case '1':	case '2':	case '3':
		  case '4':	case '5':	case '6':
			c -= '0';
			break;
		  case '0':	case 'P':	case 'p':
			c = 0;
			break;
		  default:
			putchar('');
			addch('\b');
			if (!isprint(c))
				addch('\b');
			c = -1;
			break;
		}
		refresh();
		if (c >= 0) {
			while ((c1=readch()) != '\r' && c1 != '\n' && c1 != ' ')
				if (c1 == killchar())
					return -1;
				else if (c1 == erasechar()) {
					addch('\b');
					clrtoeol();
					refresh();
					goto cont;
				} else {
					if (write(0, "", 1) != 1)
					        /*ignore*/;
                                }
			return c;
		}
cont:		;
	}
}

int
check_ext(forcomp)
bool	forcomp; {


	if (End == 700)
		if (Play == PLAYER) {
			if (getyn(EXTENSIONPROMPT)) {
extend:
				if (!forcomp)
					End = 1000;
				return TRUE;
			}
			else {
done:
				if (!forcomp)
					Finished = TRUE;
				return FALSE;
			}
		}
		else {
			PLAY	*pp, *op;
			int	i, safe, miles;

			pp = &Player[COMP];
			op = &Player[PLAYER];
			for (safe = 0, i = 0; i < NUMSAFE; i++)
				if (pp->safety[i] != S_UNKNOWN)
					safe++;
			if (safe < 2)
				goto done;
			if (op->mileage == 0 || onecard(op)
			    || (op->can_go && op->mileage >= 500))
				goto done;
			for (miles = 0, i = 0; i < NUMSAFE; i++)
				if (op->safety[i] != S_PLAYED
				    && pp->safety[i] == S_UNKNOWN)
					miles++;
			if (miles + safe == NUMSAFE)
				goto extend;
			for (miles = 0, i = 0; i < HAND_SZ; i++)
				if ((safe = pp->hand[i]) <= C_200)
					miles += Value[safe];
			if (miles + (Topcard - Deck) * 3 > 1000)
				goto extend;
			goto done;
		}
	else
		goto done;
}

/*
 *	Get a yes or no answer to the given question.  Saves are
 * also allowed.  Return TRUE if the answer was yes, FALSE if no.
 */
int
getyn(promptno)
register int	promptno; {

	char	c;

	Saved = FALSE;
	for (;;) {
		leaveok(Board, FALSE);
		prompt(promptno);
		clrtoeol();
		refresh();
		switch (c = readch()) {
		  case 'n':	case 'N':
			addch('N');
			refresh();
			leaveok(Board, TRUE);
			return FALSE;
		  case 'y':	case 'Y':
			addch('Y');
			refresh();
			leaveok(Board, TRUE);
			return TRUE;
		  case 's':	case 'S':
			addch('S');
			refresh();
			Saved = save();
			continue;
		  default:
			addstr(unctrl(c));
			refresh();
			putchar('');
			break;
		}
	}
}

static void
flush_input()
{
# ifdef	TIOCFLUSH
	static int	ioctl_args = FREAD;

	(void) ioctl(fileno(stdin), TIOCFLUSH, &ioctl_args);
# else
	fflush(stdin);
# endif
}

/*
 *	Check to see if more games are desired.  If not, and game
 * came from a saved file, make sure that they don't want to restore
 * it.  Exit appropriately.
 */
void
check_more() {

	flush_input();

	On_exit = TRUE;
	if (Player[PLAYER].total >= 5000 || Player[COMP].total >= 5000)
		if (getyn(ANOTHERGAMEPROMPT))
			return;
		else {
			/*
			 * must do accounting normally done in main()
			 */
			if (Player[PLAYER].total > Player[COMP].total)
				Player[PLAYER].games++;
			else if (Player[PLAYER].total < Player[COMP].total)
				Player[COMP].games++;
			Player[COMP].total = 0;
			Player[PLAYER].total = 0;
		}
	else
		if (getyn(ANOTHERHANDPROMPT))
			return;
	if (!Saved && getyn(SAVEGAMEPROMPT))
		if (!save())
			return;
	die();
}

int
readch()
{
	int		cnt;
	static char	c;

	for (cnt = 0; read(0, &c, 1) <= 0; cnt++)
		if (cnt > 100)
			exit(1);
	return c;
}

#ifdef CROSS
/*
 * define unctrl codes for each character
 *
 */
char *_unctrl[] = {     /* unctrl codes for ttys                */
        "^@", "^A", "^B", "^C", "^D", "^E", "^F", "^G", "^H", "^I", "^J", "^K",
        "^L", "^M", "^N", "^O", "^P", "^Q", "^R", "^S", "^T", "^U", "^V", "^W",
        "^X", "^Y", "^Z", "^[", "^\\", "^]", "^~", "^_",
        " ", "!", "\"", "#", "$",  "%", "&", "'", "(", ")", "*", "+", ",", "-",
        ".", "/", "0",  "1", "2",  "3", "4", "5", "6", "7", "8", "9", ":", ";",
        "<", "=", ">",  "?", "@",  "A", "B", "C", "D", "E", "F", "G", "H", "I",
        "J", "K", "L",  "M", "N",  "O", "P", "Q", "R", "S", "T", "U", "V", "W",
        "X", "Y", "Z",  "[", "\\", "]", "^", "_", "`", "a", "b", "c", "d", "e",
        "f", "g", "h",  "i", "j",  "k", "l", "m", "n", "o", "p", "q", "r", "s",
        "t", "u", "v",  "w", "x",  "y", "z", "{", "|", "}", "~", "^?"
};
#endif
