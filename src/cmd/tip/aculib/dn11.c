/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Routines for dialing up on DN-11
 */
#include "tip.h"
#include <fcntl.h>
#include <sys/wait.h>

static jmp_buf jmpbuf;
static int child = -1, dn;

void alarmtr(int sig)
{
    alarm(0);
    longjmp(jmpbuf, 1);
}

int dn_dialer(
    char *num, char *acu)
{
    int lt, nw;
    register int timelim;

    if (boolean(value(VERBOSE)))
        printf("\nstarting call...");
    if ((dn = open(acu, 1)) < 0) {
        if (errno == EBUSY)
            printf("line busy...");
        else
            printf("acu open error...");
        return (0);
    }
    if (setjmp(jmpbuf)) {
        kill(child, SIGKILL);
        close(dn);
        return (0);
    }
    signal(SIGALRM, alarmtr);
    timelim = 5 * strlen(num);
    alarm(timelim < 30 ? 30 : timelim);
    if ((child = fork()) == 0) {
        /*
         * ignore this stuff for aborts
         */
        signal(SIGALRM, SIG_IGN);
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        sleep(2);
        nw = write(dn, num, lt = strlen(num));
        exit(nw != lt);
    }
    /*
     * open line - will return on carrier
     */
    if ((FD = open(DV, 2)) < 0) {
        if (errno == EIO)
            printf("lost carrier...");
        else
            printf("dialup line open failed...");
        alarm(0);
        kill(child, SIGKILL);
        close(dn);
        return (0);
    }
    alarm(0);
    ioctl(dn, TIOCHPCL, 0);
    signal(SIGALRM, SIG_DFL);
    while ((nw = wait(&lt)) != child && nw != -1)
        ;
    fflush(stdout);
    close(dn);
    if (lt != 0) {
        close(FD);
        return (0);
    }
    return (1);
}

/*
 * Insurance, for some reason we don't seem to be
 *  hanging up...
 */
void dn_disconnect()
{
    sleep(2);
    if (FD > 0)
        ioctl(FD, TIOCCDTR, 0);
    close(FD);
}

void dn_abort()
{
    sleep(2);
    if (child > 0)
        kill(child, SIGKILL);
    if (dn > 0)
        close(dn);
    if (FD > 0)
        ioctl(FD, TIOCCDTR, 0);
    close(FD);
}
