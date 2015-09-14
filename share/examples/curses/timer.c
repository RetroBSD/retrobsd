#include <curses.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define centered(y,str) mvaddstr(y, (COLS - strlen(str))/2, str)

void die()
{
    alarm(0);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    clear();
    refresh();
    endwin();
    exit(0);
}

int main(int ac, char **av)
{
    int     move_col,
            msec;
    int     time_line,
            move_line;
    long    num_start_time,
            num_now_time;
    char    str_start_time[60],
            str_now_time[60];

    msec = 0;
    if (ac > 1) {
        msec = atoi(av[1]);
	if (msec < 10)
	    msec = 10;
	else if (msec > 1000)
	    msec = 1000;
    }
    if (! initscr())
	exit(1);

    signal(SIGINT, die);
    signal(SIGQUIT, die);
    signal(SIGHUP, die);
    signal(SIGTERM, die);

    box(stdscr, '*', '*');

    time(&num_start_time);
    strcpy(str_start_time, ctime(&num_start_time));
    *(index(str_start_time, '\n')) = '\0';

    centered(3, "*** Test started at: ***");
    centered(4, str_start_time);

    move_col = 1;
    move_line = (LINES - 2) / 2;
    time_line = move_line + (LINES - move_line - 1) / 2;

    while (TRUE) {
	time(&num_now_time);
	strcpy(str_now_time, ctime(&num_now_time));
	*(index(str_now_time, '\n')) = '\0';

	centered(time_line, str_now_time);

	mvaddstr(move_line, move_col, "<-*->");

	refresh();

	mvaddstr(move_line, move_col, "     ");
	if (++move_col >= (COLS - 6))
	    move_col = 1;

	if (msec)
	    usleep(msec * 1000);
    }
}
