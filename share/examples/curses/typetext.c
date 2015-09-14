#include <curses.h>
#include <signal.h>
#include <ctype.h>
#include <stdlib.h>

#define CNTRL(c)        ((c) &037)

void die(int sig)
{
    signal(sig, SIG_IGN);
    move((LINES - 1), 0);
    refresh();
    endwin();
    exit(0);
}

int main()
{
    WINDOW *win, *boxing;
    int c;
    int x, y;

    initscr();

    signal(SIGINT, die);
    noecho();
    crmode();

    win = subwin(stdscr,
	    LINES / 2,
	    COLS / 2,
	    LINES / 4,
	    COLS / 4);
    scrollok(win, TRUE);

    boxing = subwin(stdscr,
	    LINES / 2 + 2,
	    COLS / 2 + 2,
	    LINES / 4 - 1,
	    COLS / 4 - 1);

    box(boxing, '!', '-');
    refresh();

    wmove(win, 0, 0);
    wrefresh(win);

    while ((c = wgetch(win)) != '#') {
	if (iscntrl(c)) {
	    switch (c) {
            case CNTRL('E'):
                werase(win);
                wrefresh(win);
                continue;

            case CNTRL('R'):
                wrefresh(curscr);
                continue;

            case CNTRL('['):
                getyx(win, y, x);
                c = wgetch(win);
                if (c == '[' || c == 'O')
                    c = wgetch(win);
                switch (c) {
                case 'H':
                    x = 0;
                    y = 0;
                    goto change;
                case 'A':
                    y--;
                    goto change;
                case 'B':
                    y++;
                    goto change;
                case 'C':
                    x++;
                    goto change;
                case 'D':
                    x--;
change:
                    if (x >= win->_maxx) {
                        x = 0;
                        y++;
                    }
                    if (y >= win->_maxy)
                        y = 0;
                    wmove(win, y, x);
                    wrefresh(win);
                    continue;
                default:
                    break;
                }
                break;
            default:
                continue;
	    }
        }
	waddch(win, c);
	wrefresh(win);
    }
    die(SIGINT);
}
