/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "tip.h"
#include <fcntl.h>

extern char *getremote();
extern char *rindex();

static  jmp_buf deadline;
static  int deadfl;

void
dead(int sig)
{
    deadfl = 1;
    longjmp(deadline, 1);
}

int hunt(name)
    char *name;
{
    register char *cp;
    sig_t f;

    f = signal(SIGALRM, dead);
    while ((cp = getremote(name))) {
        printf("hunt: cp = %s\n",cp);
        deadfl = 0;
        uucplock = rindex(cp, '/')+1;
        printf("hunt: uucplock = %s\n",uucplock);
        if (mlock(uucplock) < 0) {
            printf("hunt: mlock(uucplock)<0\n");
            delock(uucplock);
            continue;
        }
        /*
         * Straight through call units, such as the BIZCOMP,
         * VADIC and the DF, must indicate they're hardwired in
         *  order to get an open file descriptor placed in FD.
         * Otherwise, as for a DN-11, the open will have to
         *  be done in the "open" routine.
         */
        if (!HW)
            break;
        if (setjmp(deadline) == 0) {
            alarm(10);
            FD = open(cp, O_RDWR);
        }
        alarm(0);
        if (FD < 0) {
            perror(cp);
            deadfl = 1;
        }
        if (!deadfl) {
            ioctl(FD, TIOCEXCL, 0);
            ioctl(FD, TIOCHPCL, 0);
            signal(SIGALRM, SIG_DFL);
            return ((int)cp);
        }
        delock(uucplock);
    }
    signal(SIGALRM, f);
    return (deadfl ? -1 : (int)cp);
}
