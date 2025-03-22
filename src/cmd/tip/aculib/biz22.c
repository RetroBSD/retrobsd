/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdlib.h>
#include <string.h>
#include "tip.h"


#define DISCONNECT_CMD  "\20\04"    /* disconnection string */

static  int timeout = 0;
static  jmp_buf timeoutbuf;
static int detect(register char *s);
static int cmd(register char *s);

void biz22_disconnect()
{
    int rw = 2;

    write(FD, DISCONNECT_CMD, 4);
    sleep(2);
    ioctl(FD, TIOCFLUSH, &rw);
}

/*
 * Dial up on a BIZCOMP Model 1022 with either
 *  tone dialing (mod = "V")
 *  pulse dialing (mod = "W")
 */
static int
biz_dialer(
    char *num, char *mod)
{
    register int connected = 0;
    char cbuf[40];

    if (boolean(value(VERBOSE)))
        printf("\nstarting call...");
    /*
     * Disable auto-answer and configure for tone/pulse
     *  dialing
     */
    if (cmd("\02K\r")) {
        printf("can't initialize bizcomp...");
        return (0);
    }
    strcpy(cbuf, "\02.\r");
    cbuf[1] = *mod;
    if (cmd(cbuf)) {
        printf("can't set dialing mode...");
        return (0);
    }
    strcpy(cbuf, "\02D");
    strcat(cbuf, num);
    strcat(cbuf, "\r");
    write(FD, cbuf, strlen(cbuf));
    if (!detect("7\r")) {
        printf("can't get dial tone...");
        return (0);
    }
    if (boolean(value(VERBOSE)))
        printf("ringing...");
    /*
     * The reply from the BIZCOMP should be:
     *  2 \r or 7 \r    failure
     *  1 \r        success
     */
    connected = detect("1\r");
#ifdef ACULOG
    if (timeout) {
        char line[80];

        sprintf(line, "%d second dial timeout",
            number(value(DIALTIMEOUT)));
        logent(value(HOST), num, "biz1022", line);
    }
#endif
    if (timeout)
        biz22_disconnect(); /* insurance */
    return (connected);
}

int biz22w_dialer(
    char *num, char *acu)
{
    return (biz_dialer(num, "W"));
}

int biz22f_dialer(
    char *num, char *acu)
{
    return (biz_dialer(num, "V"));
}

void biz22_abort()
{
    write(FD, "\02", 1);
}

static void
sigALRM(int i)
{

    timeout = 1;
    longjmp(timeoutbuf, 1);
}

static int
cmd(
    register char *s)
{
    char c;
    sig_t f;

    write(FD, s, strlen(s));
    f = signal(SIGALRM, sigALRM);
    if (setjmp(timeoutbuf)) {
        biz22_abort();
        signal(SIGALRM, f);
        return (1);
    }
    alarm(number(value(DIALTIMEOUT)));
    read(FD, &c, 1);
    alarm(0);
    signal(SIGALRM, f);
    c &= 0177;
    return (c != '\r');
}

static int detect(
    register char *s)
{
    char c;
    sig_t f;

    f = signal(SIGALRM, sigALRM);
    timeout = 0;
    while (*s) {
        if (setjmp(timeoutbuf)) {
            biz22_abort();
            break;
        }
        alarm(number(value(DIALTIMEOUT)));
        read(FD, &c, 1);
        alarm(0);
        c &= 0177;
        if (c != *s++)
            return (0);
    }
    signal(SIGALRM, f);
    return (timeout == 0);
}
