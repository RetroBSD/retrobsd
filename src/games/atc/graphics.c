/*
 * Copyright (c) 1987 by Ed James, UC Berkeley.  All rights reserved.
 *
 * Copy permission is hereby granted provided that this notice is
 * retained on all partial or complete copies.
 *
 * For more info on this and all of my stuff, mail edjames@berkeley.edu.
 */

#include "include.h"
#ifdef SYSV
#include <errno.h>
#endif

#define C_TOPBOTTOM		'-'
#define C_LEFTRIGHT		'|'
#define C_AIRPORT		'='
#define C_LINE			'+'
#define C_BACKROUND		'.'
#define C_BEACON		'*'
#define C_CREDIT		'*'

char *command(PLANE *pp);

WINDOW	*radar, *cleanradar, *credit, *input, *planes;

int getAChar()
{
#ifdef BSD
	return (getchar());
#endif
#ifdef SYSV
	int c;

	while ((c = getchar()) == -1 && errno == EINTR) ;
	return(c);
#endif
}

void erase_all()
{
	PLANE	*pp;

	for (pp = air.head; pp != NULL; pp = pp->next) {
		wmove(cleanradar, pp->ypos, pp->xpos * 2);
		wmove(radar, pp->ypos, pp->xpos * 2);
		waddch(radar, winch(cleanradar));
		wmove(cleanradar, pp->ypos, pp->xpos * 2 + 1);
		wmove(radar, pp->ypos, pp->xpos * 2 + 1);
		waddch(radar, winch(cleanradar));
	}
}

void draw_all()
{
	PLANE	*pp;

	for (pp = air.head; pp != NULL; pp = pp->next) {
		if (pp->status == S_MARKED)
			wstandout(radar);
		wmove(radar, pp->ypos, pp->xpos * 2);
		waddch(radar, name(pp));
		waddch(radar, '0' + pp->altitude);
		if (pp->status == S_MARKED)
			wstandend(radar);
	}
	wrefresh(radar);
	planewin();
	wrefresh(input);		/* return cursor */
	fflush(stdout);
}

void init_gr()
{
	static char	buffer[BUFSIZ];

	initscr();
	setbuf(stdout, buffer);
	input = newwin(INPUT_LINES, COLS - PLANE_COLS, LINES - INPUT_LINES, 0);
	credit = newwin(INPUT_LINES, PLANE_COLS, LINES - INPUT_LINES,
		COLS - PLANE_COLS);
	planes = newwin(LINES - INPUT_LINES, PLANE_COLS, 0, COLS - PLANE_COLS);
}

void draw_line(WINDOW *w, int x, int y, int lx, int ly, char *s)
{
	int	dx, dy;

	dx = SGN(lx - x);
	dy = SGN(ly - y);
	for (;;) {
		wmove(w, y, x * 2);
		waddstr(w, s);
		if (x == lx && y == ly)
			break;
		x += dx;
		y += dy;
	}
}

void setup_screen(C_SCREEN *scp)
{
	register int	i, j;
	char		str[3], *airstr;

	str[2] = '\0';

	if (radar != NULL)
		delwin(radar);
	radar = newwin(scp->height, scp->width * 2, 0, 0);

	if (cleanradar != NULL)
		delwin(cleanradar);
	cleanradar = newwin(scp->height, scp->width * 2, 0, 0);

	/* minus one here to prevent a scroll */
	for (i = 0; i < PLANE_COLS - 1; i++) {
		wmove(credit, 0, i);
		waddch(credit, C_CREDIT);
		wmove(credit, INPUT_LINES - 1, i);
		waddch(credit, C_CREDIT);
	}
	wmove(credit, INPUT_LINES / 2, 1);
	waddstr(credit, AUTHOR_STR);

	for (i = 1; i < scp->height - 1; i++) {
		for (j = 1; j < scp->width - 1; j++) {
			wmove(radar, i, j * 2);
			waddch(radar, C_BACKROUND);
		}
	}

	/*
	 * Draw the lines first, since people like to draw lines
	 * through beacons and exit points.
	 */
	str[0] = C_LINE;
	for (i = 0; i < scp->num_lines; i++) {
		str[1] = ' ';
		draw_line(radar, scp->line[i].p1.x, scp->line[i].p1.y,
			scp->line[i].p2.x, scp->line[i].p2.y, str);
	}

	str[0] = C_TOPBOTTOM;
	str[1] = C_TOPBOTTOM;
	wmove(radar, 0, 0);
	for (i = 0; i < scp->width - 1; i++)
		waddstr(radar, str);
	waddch(radar, C_TOPBOTTOM);

	str[0] = C_TOPBOTTOM;
	str[1] = C_TOPBOTTOM;
	wmove(radar, scp->height - 1, 0);
	for (i = 0; i < scp->width - 1; i++)
		waddstr(radar, str);
	waddch(radar, C_TOPBOTTOM);

	for (i = 1; i < scp->height - 1; i++) {
		wmove(radar, i, 0);
		waddch(radar, C_LEFTRIGHT);
		wmove(radar, i, (scp->width - 1) * 2);
		waddch(radar, C_LEFTRIGHT);
	}

	str[0] = C_BEACON;
	for (i = 0; i < scp->num_beacons; i++) {
		str[1] = '0' + i;
		wmove(radar, scp->beacon[i].y, scp->beacon[i].x * 2);
		waddstr(radar, str);
	}

	for (i = 0; i < scp->num_exits; i++) {
		wmove(radar, scp->exit[i].y, scp->exit[i].x * 2);
		waddch(radar, '0' + i);
	}

	airstr = "^?>?v?<?";
	for (i = 0; i < scp->num_airports; i++) {
		str[0] = airstr[scp->airport[i].dir];
		str[1] = '0' + i;
		wmove(radar, scp->airport[i].y, scp->airport[i].x * 2);
		waddstr(radar, str);
	}

	overwrite(radar, cleanradar);
	wrefresh(radar);
	wrefresh(credit);
	fflush(stdout);
}

void ioclrtoeol(int pos)
{
	wmove(input, 0, pos);
	wclrtoeol(input);
	wrefresh(input);
	fflush(stdout);
}

void iomove(int pos)
{
	wmove(input, 0, pos);
	wrefresh(input);
	fflush(stdout);
}

void ioaddstr(int pos, char *str)
{
	wmove(input, 0, pos);
	waddstr(input, str);
	wrefresh(input);
	fflush(stdout);
}

void ioclrtobot()
{
	wclrtobot(input);
	wrefresh(input);
	fflush(stdout);
}

void ioerror(int pos, int len, char *str)
{
	int	i;

	wmove(input, 1, pos);
	for (i = 0; i < len; i++)
		waddch(input, '^');
	wmove(input, 2, 0);
	waddstr(input, str);
	wrefresh(input);
	fflush(stdout);
}

void quit(int sig)
{
	int			c, y, x;
#ifdef BSD
	struct itimerval	itv;
#endif

	getyx(input, y, x);
	wmove(input, 2, 0);
	waddstr(input, "Really quit? (y/n) ");
	wclrtobot(input);
	wrefresh(input);
	fflush(stdout);

	c = getchar();
	if (c == EOF || c == 'y') {
		/* disable timer */
#ifdef BSD
		itv.it_value.tv_sec = 0;
		itv.it_value.tv_usec = 0;
		setitimer(ITIMER_REAL, &itv, NULL);
#endif
#ifdef SYSV
		alarm(0);
#endif
		fflush(stdout);
		clear();
		refresh();
		endwin();
		log_score(0);
		exit(0);
	}
	wmove(input, 2, 0);
	wclrtobot(input);
	wmove(input, y, x);
	wrefresh(input);
	fflush(stdout);
}

void planewin()
{
	PLANE	*pp;
	int	warning = 0;

#ifdef BSD
	wclear(planes);
#endif

	wmove(planes, 0,0);

#ifdef SYSV
	wclrtobot(planes);
#endif
	wprintw(planes, "Time: %-4d Safe: %d", clocktick, safe_planes);
	wmove(planes, 2, 0);

	waddstr(planes, "pl dt  comm");
	for (pp = air.head; pp != NULL; pp = pp->next) {
		if (waddch(planes, '\n') == ERR) {
			warning++;
			break;
		}
		waddstr(planes, command(pp));
	}
	waddch(planes, '\n');
	for (pp = ground.head; pp != NULL; pp = pp->next) {
		if (waddch(planes, '\n') == ERR) {
			warning++;
			break;
		}
		waddstr(planes, command(pp));
	}
	if (warning) {
		wmove(planes, LINES - INPUT_LINES - 1, 0);
		waddstr(planes, "---- more ----");
		wclrtoeol(planes);
	}
	wrefresh(planes);
	fflush(stdout);
}

void loser(PLANE *p, char *s)
{
	int			c;
#ifdef BSD
	struct itimerval	itv;
#endif

	/* disable timer */
#ifdef BSD
	itv.it_value.tv_sec = 0;
	itv.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &itv, NULL);
#endif
#ifdef SYSV
	alarm(0);
#endif

	wmove(input, 0, 0);
	wclrtobot(input);
	wprintw(input, "Plane '%c' %s\n\nHit space for top players list...",
		name(p), s);
	wrefresh(input);
	fflush(stdout);
	while ((c = getchar()) != EOF && c != ' ')
		;
	clear();	/* move to top of screen */
	refresh();
	endwin();
	log_score(0);
	exit(0);
}

void redraw()
{
	clear();
	refresh();

	touchwin(radar);
	wrefresh(radar);
	touchwin(planes);
	wrefresh(planes);
	touchwin(credit);
	wrefresh(credit);

	/* refresh input last to get cursor in right place */
	touchwin(input);
	wrefresh(input);
	fflush(stdout);
}


void done_screen()
{
	clear();
	refresh();
	endwin();	  /* clean up curses */
}
