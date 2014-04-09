/*
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */

/* The following is a curses emulation package suitable for the rogue program
 * in which it is included.  No other suitability is claimed or suspected.
 * Only those routines currently needed by this rogue program are included.
 * This is being provided for those systems that don't have a suitable
 * curses package and want to run this rogue program.
 *
 * Compile the entire program with -DCURSES to incorporate this package.
 *
 * The following is NOT supported:
 *   "%D", "%B", "%n", or "%>" inside a cursor motion (cm) termcap string.
 *   Terminals in which the cursor motion addresses the row differently from
 *       the column, as in ":cm=\E%2,%3" or ":cm=\EY%+x;%+y"
 *   Termcap database stored in the TERMCAP environ variable as returned
 *       from getenv().  Only the termcap file name can be stored there.
 *       See the comments for getenv() in machdep.c.
 *   Terminals without non-destructive backspace.  Backspace (^H) is used
 *       for cursor motion regardless of any termcap entries.
 *   The ":tc=" termcap entry is ignored.
 *
 * Suggestions:
 *   Use line-feed as your termcap "do" entry: ":do=^J", ":do=\012" or
 *      ":do=\n"  This will help cursor motion optimization.  If line-feed
 *      won't work, then a short escape sequence will do.
 */
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <curses.h>

#define DROWS 24
#define DCOLS 80

#define BS 010
#define LF 012
#define CR 015
#define ESC '\033'
#define TAB '\011'

#define ST_MASK 0x80

char terminal[DROWS][DCOLS];
char buffer[DROWS][DCOLS];

char cm_esc[16];
char cm_sep[16];
char cm_end[16];
int cm_reverse = 0;
int cm_two = 0;
int cm_three = 0;
int cm_char = 0;
short cm_inc = 0;

int screen_dirty;
int lines_dirty[DROWS];
int buf_stand_out = 0;
int term_stand_out = 0;

int LINES = DROWS;
int COLS = DCOLS;
WINDOW scr_buf;
WINDOW *curscr = &scr_buf;

int _tty_ch, _res_flg;
struct sgttyb _tty;
int _echoit, _rawmode;

char *CL = (char *) 0;
char *CM = (char *) 0;
char *UC = (char *) 0;	/* UP */
char *DO = (char *) 0;
char *VS = "";
char *VE = "";
char *TI = "";
char *TE = "";
char *SO = "";
char *SE = "";

WINDOW *stdscr = (WINDOW *) 1;

short cur_row;
short cur_col;

#ifndef ANSI
#define BUFLEN 256

static int
tc_tname(fp, term, buf)
        FILE *fp;
        char *term;
        char *buf;
{
	int i, j;
	int found = 0;
	char *fg;

	while (!found) {
		i = 0;
		fg = fgets(buf, BUFLEN, fp);
		if (fg != NULL) {
			if ((buf[0] != '#') && (buf[0] != ' ') && (buf[0] != TAB) &&
					(buf[0] != CR) && (buf[0] != LF))
                        {
				while (buf[i] && (!found)) {
					j = 0;
					while (buf[i] == term[j]) {
						i++;
						j++;
					}
					if ((!term[j]) && ((buf[i] == '|') || (buf[i] == ':'))) {
						found = 1;
					} else {
						while (buf[i] && (buf[i] != '|') && (buf[i] != ':')) {
							i++;
						}
						if (buf[i]) {
							i++;
						}
					}
				}
			}
		} else {
			break;
		}
	}
	return(found);
}

static int
is_digit(ch)
        int ch;
{
        return((ch >= '0') && (ch <= '9'));
}

static void
tc_gets(ibuf, tcstr)
        char *ibuf;
        char **tcstr;
{
	int i, j, k, n;
	char obuf[BUFLEN];

	i = 4;
	j = 0;

	while (ibuf[i] && is_digit(ibuf[i])) {
		i++;
	}

	while (ibuf[i] && (ibuf[i] != ':')) {
		if (ibuf[i] == '\\') {
			i++;
			switch(ibuf[i]) {
			case 'E':
				obuf[j] = ESC;
				i++;
				break;
			case 'n':
				obuf[j] = LF;
				i++;
				break;
			case 'r':
				obuf[j] = CR;
				i++;
				break;
			case 'b':
				obuf[j] = BS;
				i++;
				break;
			case 't':
				obuf[j] = TAB;
				i++;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				n = 0;
				k = 0;
				while (k < 3 && ibuf[i] && is_digit(ibuf[i])) {
					n = (8 * n) + (ibuf[i] - '0');
					i++;
					k++;
				}
				obuf[j] = (char) n;
				break;
			default:
				obuf[j] = ibuf[i];
				i++;
			}
		} else if (ibuf[i] == '^') {
			obuf[j] = ibuf[i+1] - 64;
			i += 2;
		} else {
			obuf[j] = ibuf[i++];
		}
		j++;
	}
	obuf[j] = 0;
	*tcstr = malloc(j + 1);
	if (! *tcstr) {
		perror("malloc");
		exit(1);
	}
	(void) strcpy(*tcstr, obuf);
}

static void
tc_gnum(ibuf, n)
        char *ibuf;
        int *n;
{
	int i, r = 0;

	i = 4;

	while (is_digit(ibuf[i])) {
		r = (r * 10) + (ibuf[i] - '0');
		i++;
	}
	*n = r;
}

static void
tc_cmget()
{
	int i = 0, j = 0, rc_spec = 0;

	while (CM[i] && (CM[i] != '%') && (j < 15)) {
		cm_esc[j++] = CM[i++];
	}
	cm_esc[j] = 0;

	while (CM[i] && (rc_spec < 2)) {
		if (CM[i] == '%') {
			i++;
			switch(CM[i]) {
			case 'd':
				rc_spec++;
				break;
			case 'i':
				cm_inc = 1;
				break;
			case '2':
				cm_two = 1;
				rc_spec++;
				break;
			case '3':
				cm_three = 1;
				rc_spec++;
				break;
			case '.':
				cm_char = 1;
				rc_spec++;
				break;
			case 'r':
				cm_reverse = 1;
				break;
			case '+':
				i++;
				cm_inc = CM[i];
				cm_char = 1;
				rc_spec++;
				break;
			}
			i++;
		} else {
			j = 0;
			while (CM[i] && (CM[i] != '%')) {
				cm_sep[j++] = CM[i++];
			}
			cm_sep[j] = 0;
		}
	}

	j = 0;
	if (rc_spec == 2) {
		while (CM[i] && (j < 15)) {
			cm_end[j++] = CM[i++];
		}
	}
	cm_end[j] = 0;
}

static void
tc_gtdata(fp, buf)
        FILE *fp;
        char *buf;
{
	int i;
	int first = 1;

	do {
		if (!first) {
			if ((buf[0] != TAB) && (buf[0] != ' ')) {
				break;
			}
		}
		first = 0;
		i = 0;
		while (buf[i]) {
			while (buf[i] && (buf[i] != ':')) {
				i++;
			}
			if (buf[i] == ':') {
				if (!strncmp(buf + i, ":cl=", 4)) {
					tc_gets(buf + i, &CL);
				} else if (!strncmp(buf + i, ":cm=", 4)) {
					tc_gets(buf + i, &CM);
				} else if (!strncmp(buf + i, ":up=", 4)) {
					tc_gets(buf + i, &UC);
				} else if (!strncmp(buf + i, ":do=", 4)) {
					tc_gets(buf + i, &DO);
				} else if (!strncmp(buf + i, ":vs=", 4)) {
					tc_gets(buf + i, &VS);
				} else if (!strncmp(buf + i, ":ve=", 4)) {
					tc_gets(buf + i, &VE);
				} else if (!strncmp(buf + i, ":ti=", 4)) {
					tc_gets(buf + i, &TI);
				} else if (!strncmp(buf + i, ":te=", 4)) {
					tc_gets(buf + i, &TE);
				} else if (!strncmp(buf + i, ":so=", 4)) {
					tc_gets(buf + i, &SO);
				} else if (!strncmp(buf + i, ":se=", 4)) {
					tc_gets(buf + i, &SE);
				} else if (!strncmp(buf + i, ":li#", 4)) {
					tc_gnum(buf + i, &LINES);
				} else if (!strncmp(buf + i, ":co#", 4)) {
					tc_gnum(buf + i, &COLS);
				}
				i++;
			}
		}
	} while (fgets(buf, BUFLEN, fp) != NULL);

	if (!CM || !CL) {
		fprintf(stderr, "Terminal and termcap must have cm and cl\n");
		exit(1);
	}
	tc_cmget();
}
#endif /* ANSI */

static void
get_term_info()
{
#ifdef ANSI
        /* Generic ANSI display. */
        LINES = DROWS;
        COLS  = DCOLS;

        CL = "\33[H\33[2J";
        UC = "\33[A";
        DO = "\12";
        SO = "\33[7m";
        SE = "\33[m";

        cm_inc = 1;
        strcpy (cm_esc, "\33[");
        strcpy (cm_sep, ";");
        strcpy (cm_end, "H");
#else
	FILE *fp;
	char *term, *tcf;
	char buf[BUFLEN];
        char *tc_file = "/etc/termcap";

        tcf = getenv("TERMCAP");
	if (tcf) {
		if (strlen(tcf) > 40) {
                        fprintf(stderr, "TERMCAP file name too long\n");
                        exit(1);
		}
		tc_file = tcf;
	}

	term = getenv("TERM");
	if (! term) {
                fprintf(stderr, "Cannot find TERM variable in environ\n");
                exit(1);
	}
	fp = fopen(tc_file, "r");
	if (! fp) {
		fprintf(stderr, "Cannot open TERMCAP file: %s\n", tc_file);
                exit(1);
	}

	if (! tc_tname(fp, term, buf)) {
		fprintf(stderr, "Cannot find TERM type: %s in TERMCAP file: %s", term,
			tc_file);
                exit(1);
	}
	tc_gtdata(fp, buf);
	fclose(fp);
#endif
}

WINDOW *
initscr()
{
        savetty();
	clear();
	get_term_info();
	printf("%s%s", TI, VS);
	return stdscr;
}

void
endwin()
{
	printf("%s%s", TE, VE);
        resetty();
}

int
wmove(w, row, col)
        WINDOW *w;
        int row, col;
{
	curscr->_cury = row;
	curscr->_curx = col;
	screen_dirty = 1;
	return 0;
}

int
waddstr(w, str)
        WINDOW *w;
        char *str;
{
	while (*str) {
		addch((int) *str++);
	}
	return 0;
}

int
waddch(w, ch)
        WINDOW *w;
        register char ch;
{
	int row, col;

	row = curscr->_cury;
	col = curscr->_curx++;

	if (buf_stand_out) {
		ch |= ST_MASK;
	}
	buffer[row][col] = (char) ch;
	lines_dirty[row] = 1;
	screen_dirty = 1;
	return 0;
}

static void
put_st_char(ch)
        register int ch;
{
	if ((ch & ST_MASK) && (!term_stand_out)) {
		ch &= ~ST_MASK;
		printf("%s%c", SO, ch);
		term_stand_out = 1;
	} else if ((!(ch & ST_MASK)) && term_stand_out) {
		printf("%s%c", SE, ch);
		term_stand_out = 0;
	} else {
		ch &= ~ST_MASK;
		putchar(ch);
	}
}

static void
put_cursor(row, col)
        register int row, col;
{
	register int i, rdif, cdif;
	int ch, t;

	rdif = (row > cur_row) ? row - cur_row : cur_row - row;
	cdif = (col > cur_col) ? col - cur_col : cur_col - col;

	if (((row > cur_row) && DO) || ((cur_row > row) && UC)) {
		if ((rdif < 4) && (cdif < 4)) {
			for (i = 0; i < rdif; i++) {
				printf("%s", ((row < cur_row) ? UC : DO));
			}
			cur_row = row;
			if (col == cur_col) {
				return;
			}
		}
	}
	if (row == cur_row) {
		if (cdif <= 6) {
		for (i = 0; i < cdif; i++) {
				ch = (col < cur_col) ? BS :
						terminal[row][cur_col + i];
				put_st_char((int) ch);
			}
			cur_row = row;
			cur_col = col;
			return;
		}
	}
	cur_row = row;
	cur_col = col;

	row += cm_inc;
	col += cm_inc;

	if (cm_reverse) {
		t = row;
		row = col;
		col = t;
	}
	if (cm_two) {
		printf("%s%02d%s%02d%s", cm_esc, row, cm_sep, col, cm_end);
	} else if (cm_three) {
		printf("%s%03d%s%03d%s", cm_esc, row, cm_sep, col, cm_end);
	} else if (cm_char) {
		printf("%s%c%s%c%s", cm_esc, row, cm_sep, col, cm_end);
	} else {
		printf("%s%d%s%d%s", cm_esc, row, cm_sep, col, cm_end);
	}
}

int
wrefresh(scr)
        WINDOW *scr;
{
	int i, col;

	printf("%s", CL);
	cur_row = cur_col = 0;

	for (i = 0; i < DROWS; i++) {
		col = 0;
		while (col < DCOLS) {
			while ((col < DCOLS) && (buffer[i][col] == ' ')) {
				col++;
			}
			if (col < DCOLS) {
				put_cursor(i, col);
			}
			while ((col < DCOLS) && (buffer[i][col] != ' ')) {
				put_st_char((int) buffer[i][col]);
				cur_col++;
				col++;
			}
		}
	}
	put_cursor(curscr->_cury, curscr->_curx);
	fflush(stdout);
	scr = scr;		/* make lint happy */
	return 0;
}

static void
clear_buffers()
{
	register int i, j;

	screen_dirty = 0;

	for (i = 0; i < DROWS; i++) {
		lines_dirty[i] = 0;
		for (j = 0; j < DCOLS; j++) {
			terminal[i][j] = ' ';
			buffer[i][j] = ' ';
		}
	}
}

int
wclear(w)
        WINDOW *w;
{
	printf("%s", CL);
	fflush(stdout);
	cur_row = cur_col = 0;
	move(0, 0);
	clear_buffers();
	return 0;
}

void
mvcur(ly, lx, y, x)
        int	ly, lx, y, x;
{
        cur_row = ly;
        cur_col = ly;
	curscr->_cury = y;
	curscr->_curx = x;
	put_cursor(curscr->_cury, curscr->_curx);
	fflush(stdout);
}

int
delwin(w)
        WINDOW *w;
{
        return 0;
}

int printw (char *fmt, ...)
{
        va_list args;
        FILE junk;
        char buf[512];

        va_start (args, fmt);
        junk._flag = _IOWRT + _IOSTRG;
        junk._ptr = buf;
        junk._cnt = 32767;
        _doprnt(fmt, args, &junk);
        va_end (args);

        putc('\0', &junk);
        waddstr(stdscr, buf);
        return 0;
}
