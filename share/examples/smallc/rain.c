/*
 * Example of using termcap library for SmallC.
 * 11/3/1980 EPS/CITHEP
 *
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdio.h>
#include <signal.h>

#define CO 80               /* number of columns */
#define LI 24               /* number of lines */

#define CL "\33[H\33[J"     /* clear the screen */
#define CM "\33[%u;%uH"     /* move the cursor to row, column */
#define BC "\b"             /* move cursor left */
#define DN "\33[B"          /* move cursor down */
#define ND " "              /* move cursor right */

int xpos[5], ypos[5];
char outbuf[BUFSIZ];

moveto(col, row)
    int col, row;
{
    printf(CM, row, col);
}

onsig(n)
    int n;
{
    moveto(0, LI - 1);
    fflush(stdout);
    _exit(0);
}

main()
{
    int x, y, j;

    setbuf(stdout, outbuf);
    for (j = SIGHUP; j <= SIGTERM; j++)
        if (signal(j, SIG_IGN) != SIG_IGN)
            signal(j, onsig);

    fputs(CL, stdout);
    fflush(stdout);
    for (j = 5; --j >= 0; ) {
        xpos[j] = 2 + rand() % (CO - 4);
        ypos[j] = 2 + rand() % (LI - 4);
    }
    for (j = 0; ; ) {
        x = 2 + rand() % (CO - 4);
        y = 2 + rand() % (LI - 4);
        moveto(x, y);
        putchar('.');
        moveto(xpos[j], ypos[j]);
        putchar('o');
        if (j == 0)
            j = 4;
        else
            --j;
        moveto(xpos[j], ypos[j]);
        putchar('O');
        if (j == 0)
            j = 4;
        else
            --j;
        moveto(xpos[j], ypos[j]-1);
        putchar('-');
        fputs(DN, stdout);
        fputs(BC, stdout);
        fputs(BC, stdout);
        fputs("|.|", stdout);
        fputs(DN, stdout);
        fputs(BC, stdout);
        fputs(BC, stdout);
        putchar('-');
        if (j == 0)
            j = 4;
        else
            --j;
        moveto(xpos[j], ypos[j]-2);
        putchar('-');
        fputs(DN, stdout);
        fputs(BC, stdout);
        fputs(BC, stdout);
        fputs("/ \\", stdout);
        moveto(xpos[j]-2, ypos[j]);
        fputs("| O |", stdout);
        moveto(xpos[j]-1, ypos[j]+1);
        fputs("\\ /", stdout);
        fputs(DN, stdout);
        fputs(BC, stdout);
        fputs(BC, stdout);
        putchar('-');
        if (j == 0)
            j = 4;
        else
            --j;
        moveto(xpos[j], ypos[j]-2);
        putchar(' ');
        fputs(DN, stdout);
        fputs(BC, stdout);
        fputs(BC, stdout);
        putchar(' ');
        fputs(ND, stdout);
        putchar(' ');
        moveto(xpos[j]-2, ypos[j]);
        putchar(' ');
        fputs(ND, stdout);
        putchar(' ');
        fputs(ND, stdout);
        putchar(' ');
        moveto(xpos[j]-1, ypos[j]+1);
        putchar(' ');
        fputs(ND, stdout);
        putchar(' ');
        fputs(DN, stdout);
        fputs(BC, stdout);
        fputs(BC, stdout);
        putchar(' ');
        xpos[j] = x;
        ypos[j] = y;
        fflush(stdout);
        usleep(100000);
    }
}
