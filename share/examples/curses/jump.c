#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define W_LINES 7
#define W_COLS  11

void die(int sig)
{
    signal(sig, SIG_IGN);
    werase(curscr);
    wmove(curscr, 0, 0);
    wrefresh(curscr);
    endwin();
    exit(0);
}

int main()
{
    WINDOW *win, *blank;
    int x, y;

    if (! initscr()) {
        fprintf(stderr, "Sorry, unknown terminal.\n");
        exit(1);
    }

    signal(SIGINT, die);
    signal(SIGQUIT, die);
    signal(SIGHUP, die);

    noecho();

    //delwin(stdscr);

    win = newwin(W_LINES, W_COLS, 0, 0);
    blank = newwin(W_LINES, W_COLS, 0, 0);

    box(win, '*', '*');
    mvwaddstr(win, W_LINES/2, (W_COLS - strlen("RetroBSD"))/2, "RetroBSD");

    srand(time(0));

    for (;;) {
        x = rand() % (COLS - W_COLS);
        y = rand() % (LINES - W_LINES);
        wrefresh(blank);
        if (mvwin(win, y, x) == OK) {
            wrefresh(win);
            usleep(500000);
            mvwin(blank, y, x);
        }
    }
}
