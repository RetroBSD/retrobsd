/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Routines for calling up on a Vadic 3451 Modem
 */
#include <string.h>
#include "tip.h"

static  jmp_buf Sjbuf;
static int prefix(register char *s1, register char *s2);
static int notin(char *sh, char *lg);
static int expect(register char *cp);

static void
vawrite(
    register char *cp,
    int delay)
{
    for (; *cp; sleep(delay), cp++)
        write(FD, cp, 1);
}

int v3451_dialer(
    register char *num,
    char *acu)
{
    int ok;
    sig_t func;
    int slow = number(value(BAUDRATE)) < 1200, rw = 2;
    char phone[50];
#ifdef ACULOG
    char line[80];
#endif

    /*
     * Get in synch
     */
    vawrite("I\r", 1 + slow);
    vawrite("I\r", 1 + slow);
    vawrite("I\r", 1 + slow);
    vawrite("\005\r", 2 + slow);
    if (!expect("READY")) {
        printf("can't synchronize with vadic 3451\n");
#ifdef ACULOG
        logent(value(HOST), num, "vadic", "can't synch up");
#endif
        return (0);
    }
    ioctl(FD, TIOCHPCL, 0);
    sleep(1);
    vawrite("D\r", 2 + slow);
    if (!expect("NUMBER?")) {
        printf("Vadic will not accept dial command\n");
#ifdef ACULOG
        logent(value(HOST), num, "vadic", "will not accept dial");
#endif
        return (0);
    }
    strcpy(phone, num);
    strcat(phone, "\r");
    vawrite(phone, 1 + slow);
    if (!expect(phone)) {
        printf("Vadic will not accept phone number\n");
#ifdef ACULOG
        logent(value(HOST), num, "vadic", "will not accept number");
#endif
        return (0);
    }
    func = signal(SIGINT,SIG_IGN);
    /*
     * You cannot interrupt the Vadic when its dialing;
     * even dropping DTR does not work (definitely a
     * brain damaged design).
     */
    vawrite("\r", 1 + slow);
    vawrite("\r", 1 + slow);
    if (!expect("DIALING:")) {
        printf("Vadic failed to dial\n");
#ifdef ACULOG
        logent(value(HOST), num, "vadic", "failed to dial");
#endif
        return (0);
    }
    if (boolean(value(VERBOSE)))
        printf("\ndialing...");
    ok = expect("ON LINE");
    signal(SIGINT, func);
    if (!ok) {
        printf("call failed\n");
#ifdef ACULOG
        logent(value(HOST), num, "vadic", "call failed");
#endif
        return (0);
    }
    ioctl(FD, TIOCFLUSH, &rw);
    return (1);
}

void v3451_disconnect()
{
    close(FD);
}

void v3451_abort()
{
    close(FD);
}

static void
alarmtr()
{
    longjmp(Sjbuf, 1);
}

static int
expect(
    register char *cp)
{
    char buf[300];
    register char *rp = buf;
    int timeout = 30, online = 0;

    if (strcmp(cp, "\"\"") == 0)
        return (1);
    *rp = 0;
    /*
     * If we are waiting for the Vadic to complete
     * dialing and get a connection, allow more time
     * Unfortunately, the Vadic times out 24 seconds after
     * the last digit is dialed
     */
    online = strcmp(cp, "ON LINE") == 0;
    if (online)
        timeout = number(value(DIALTIMEOUT));
    signal(SIGALRM, alarmtr);
    if (setjmp(Sjbuf))
        return (0);
    alarm(timeout);
    while (notin(cp, buf) && rp < buf + sizeof (buf) - 1) {
        if (online && notin("FAILED CALL", buf) == 0)
            return (0);
        if (read(FD, rp, 1) < 0) {
            alarm(0);
            return (0);
        }
        if (*rp &= 0177)
            rp++;
        *rp = '\0';
    }
    alarm(0);
    return (1);
}

static int
notin(
    char *sh, char *lg)
{

    for (; *lg; lg++)
        if (prefix(sh, lg))
            return (0);
    return (1);
}

static int
prefix(
    register char *s1, char *s2)
{
    register char c;

    while ((c = *s1++) == *s2++)
        if (c == '\0')
            return (1);
    return (c == '\0');
}
