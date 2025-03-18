/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Routines for calling up on a Hayes Modem
 * (based on the old VenTel driver).
 * The modem is expected to be strapped for "echo".
 * Also, the switches enabling the DTR and CD lines
 * must be set correctly.
 * NOTICE:
 * The easy way to hang up a modem is always simply to
 * clear the DTR signal. However, if the +++ sequence
 * (which switches the modem back to local mode) is sent
 * before modem is hung up, removal of the DTR signal
 * has no effect (except that it prevents the modem from
 * recognizing commands).
 * (by Helge Skrivervik, Calma Company, Sunnyvale, CA. 1984)
 */
/*
 * TODO:
 * It is probably not a good idea to switch the modem
 * state between 'verbose' and terse (status messages).
 * This should be kicked out and we should use verbose
 * mode only. This would make it consistent with normal
 * interactive use thru the command 'tip dialer'.
 */
#include "tip.h"

#define min(a,b)    ((a < b) ? a : b)

static  int timeout = 0;
static  jmp_buf timeoutbuf;
static  char gobble(register char *match);
#define DUMBUFLEN   40
static char dumbuf[DUMBUFLEN];

#define DIALING     1
#define IDLE        2
#define CONNECTED   3
#define FAILED      4
static  int state = IDLE;

static void sigALRM(int sig)
{
    printf("\07timeout waiting for reply\n\r");
    timeout = 1;
    longjmp(timeoutbuf, 1);
}

#define MAXRETRY    5

int hay_sync()
{
    int len, retry = 0;
    long llen;

    while (retry++ <= MAXRETRY) {
        write(FD, "AT\r", 3);
        sleep(1);
        ioctl(FD, FIONREAD, &llen);
        len = llen;
        if (len) {
            len = read(FD, dumbuf, min(len, DUMBUFLEN));
            if (strchr(dumbuf, '0') ||
            (strchr(dumbuf, 'O') && strchr(dumbuf, 'K')))
                return(1);
#ifdef DEBUG
            dumbuf[len] = '\0';
            printf("hay_sync: (\"%s\") %d\n\r", dumbuf, retry);
#endif
        }
        ioctl(FD, TIOCCDTR, 0);
        ioctl(FD, TIOCSDTR, 0);
    }
    printf("Cannot synchronize with hayes...\n\r");
    return(0);
}

void error_rep(
    register char c)
{
    printf("\n\r");
    switch (c) {

    case '0':
        printf("OK");
        break;

    case '1':
        printf("CONNECT");
        break;

    case '2':
        printf("RING");
        break;

    case '3':
        printf("NO CARRIER");
        break;

    case '4':
        printf("ERROR in input");
        break;

    case '5':
        printf("CONNECT 1200");
        break;

    default:
        printf("Unknown Modem error: %c (0x%x)", c, c);
    }
    printf("\n\r");
}

/*
 * set modem back to normal verbose status codes.
 */
void goodbye()
{
#ifdef DEBUG
    int len;
#endif
    int zero = 0;
    long llen;
    char c;

    ioctl(FD, TIOCFLUSH, &zero);    /* get rid of trash */
    if (hay_sync()) {
        sleep(1);
#ifndef DEBUG
        ioctl(FD, TIOCFLUSH, &zero);
#endif
        write(FD, "ATH0\r", 5);     /* insurance */
#ifndef DEBUG
        c = gobble("03");
        if (c != '0' && c != '3') {
            printf("cannot hang up modem\n\r");
            printf("please use 'tip dialer' to make sure the line is hung up\n\r");
        }
#endif
        sleep(1);
        ioctl(FD, FIONREAD, &llen);
#ifdef DEBUG
        len = llen;
        printf("goodbye1: len=%d -- ", len);
        int rlen = read(FD, dumbuf, min(len, DUMBUFLEN));
        dumbuf[rlen] = '\0';
        printf("read (%d): %s\r\n", rlen, dumbuf);
#endif
        write(FD, "ATV1\r", 5);
        sleep(1);
#ifdef DEBUG
        ioctl(FD, FIONREAD, &llen);
        len = llen;
        printf("goodbye2: len=%d -- ", len);
        rlen = read(FD, dumbuf, min(len, DUMBUFLEN));
        dumbuf[rlen] = '\0';
        printf("read (%d): %s\r\n", rlen, dumbuf);
#endif
    }
    ioctl(FD, TIOCFLUSH, &zero);    /* clear the input buffer */
    ioctl(FD, TIOCCDTR, 0);     /* clear DTR (insurance) */
    close(FD);
}

void hay_disconnect()
{
    /* first hang up the modem*/
#ifdef DEBUG
    printf("\rdisconnecting modem....\n\r");
#endif
    ioctl(FD, TIOCCDTR, 0);
    sleep(1);
    ioctl(FD, TIOCSDTR, 0);
    goodbye();
}

int hay_dialer(
    register char *num,
    char *acu)
{
    register int connected = 0;
    int zero = 0;
    char dummy;
#ifdef ACULOG
    char line[80];
#endif
    if (hay_sync() == 0)        /* make sure we can talk to the modem */
        return(0);
    if (boolean(value(VERBOSE)))
        printf("\ndialing...");
    fflush(stdout);
    ioctl(FD, TIOCHPCL, 0);
    ioctl(FD,TIOCFLUSH, &zero);
    write(FD, "ATV0E0X0\r", 9); /* numeric codes,noecho,base cmds */
    sleep(1);
    ioctl(FD, TIOCFLUSH, &zero);    /* get rid of garbage */

    write(FD, "ATDT", 4);   /* send dial command */
    write(FD, num, strlen(num));
    state = DIALING;
    write(FD, "\r", 1);
    connected = 0;
    if ((dummy = gobble("01234")) != '1')
        error_rep(dummy);
    else
        connected = 1;
    if (connected)
        state = CONNECTED;
    else {
        state = FAILED;
        return (connected); /* lets get out of here.. */
    }
    ioctl(FD, TIOCFLUSH, &zero);
#ifdef ACULOG
    if (timeout) {
        sprintf(line, "%d second dial timeout",
            number(value(DIALTIMEOUT)));
        logent(value(HOST), num, "hayes", line);
    }
#endif
    if (timeout)
        hay_disconnect();   /* insurance */
    return (connected);
}

void hay_abort()
{
    write(FD, "\r", 1); /* send anything to abort the call */
    hay_disconnect();
}

static char
gobble(
    register char *match)
{
    char c;
    sig_t f;
    int i, status = 0;

    f = signal(SIGALRM, sigALRM);
    timeout = 0;
#ifdef DEBUG
    printf("\ngobble: waiting for %s\n", match);
#endif
    do {
        if (setjmp(timeoutbuf)) {
            signal(SIGALRM, f);
            return (0);
        }
        alarm(number(value(DIALTIMEOUT)));
        read(FD, &c, 1);
        alarm(0);
        c &= 0177;
#ifdef DEBUG
        printf("%c 0x%x ", c, c);
#endif
        for (i = 0; i < strlen(match); i++)
            if (c == match[i])
                status = c;
    } while (status == 0);
    signal(SIGALRM, SIG_DFL);
#ifdef DEBUG
    printf("\n");
#endif
    return (status);
}
