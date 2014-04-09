/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Routines for calling up on a Penril Modem
 * The Penril is expected to be strapped for local echo (just like uucp)
 */
#include "tip.h"

#define MAXRETRY    5

static  void sigALRM(int);
static  int timeout = 0;
static  jmp_buf buftimeout;
static int pensync(int fd);
static int gobble(register char match, char response[]);

/*
 * some sleep calls have been replaced by this macro
 * because some penril modems require two <cr>s in less than
 * a second in order to 'wake up'... yes, it is dirty...
 */
#define delay(num,denom) busyloop(CPUSPEED*num/denom)
#define CPUSPEED 1000000    /* VAX 780 is 1MIPS */

void busyloop(n)
{
    while (--n > 0)
        ;
}

void pen_disconnect()
{
    close(FD);
}

static void
echo(s)
    register char *s;
{
    char c;

    while ((c = *s++)) switch (c) {

    case '$':
        read(FD, &c, 1);
        s++;
        break;

    case '#':
        c = *s++;
        write(FD, &c, 1);
        break;

    default:
        write(FD, &c, 1);
        read(FD, &c, 1);
    }
}

int pen_dialer(num, acu)
    register char *num;
    char *acu;
{
    register char *cp;
    register int connected = 0;
    char *msg, *index(), line[80];

    /*
     * Get in synch with a couple of carriage returns
     */
    if (!pensync(FD)) {
        printf("can't synchronize with penril\n");
#ifdef ACULOG
        logent(value(HOST), num, "penril", "can't synch up");
#endif
        return (0);
    }
    if (boolean(value(VERBOSE)))
        printf("\ndialing...");
    fflush(stdout);
    ioctl(FD, TIOCHPCL, 0);
    echo("#k$\r$\n$D$I$A$L$:$ ");
    for (cp = num; *cp; cp++) {
        delay(1, 10);
        write(FD, cp, 1);
    }
    delay(1, 10);
    write(FD, "\r", 1);
    gobble('\n', line);
    if (gobble('\n', line))
        connected = gobble('O', line) && gobble('K', line);
    ioctl(FD, TIOCFLUSH);
#ifdef ACULOG
    if (timeout) {
        sprintf(line, "%d second dial timeout",
            number(value(DIALTIMEOUT)));
        logent(value(HOST), num, "penril", line);
    }
#endif
    if (timeout)
        pen_disconnect();   /* insurance */
    if (connected || timeout || !boolean(value(VERBOSE)))
        return (connected);
    /* call failed, parse response for user */
    cp = index(line, '\r');
    if (cp)
        *cp = '\0';
    for (cp = line; (cp = index(cp, ' ')); cp++)
        if (cp[1] == ' ')
            break;
    if (cp) {
        while (*cp == ' ')
            cp++;
        msg = cp;
        while (*cp) {
            if (isupper(*cp))
                *cp = tolower(*cp);
            cp++;
        }
        printf("%s...", msg);
    }
    return (connected);
}

void pen_abort()
{
    write(FD, "\03", 1);
    close(FD);
}

static void
sigALRM(int i)
{
    printf("\07timeout waiting for reply\n");
    timeout = 1;
    longjmp(buftimeout, 1);
}

static int
gobble(match, response)
    register char match;
    char response[];
{
    register char *cp = response;
    char c;
    sig_t f;

    f = signal(SIGALRM, sigALRM);
    timeout = 0;
    do {
        if (setjmp(buftimeout)) {
            signal(SIGALRM, f);
            *cp = '\0';
            return (0);
        }
        alarm(number(value(DIALTIMEOUT)));
        read(FD, cp, 1);
        alarm(0);
        c = (*cp++ &= 0177);
#ifdef notdef
        if (boolean(value(VERBOSE)))
            putchar(c);
#endif
    } while (c != '\n' && c != match);
    signal(SIGALRM, SIG_DFL);
    *cp = '\0';
    return (c == match);
}

#define min(a,b)    ((a)>(b)?(b):(a))
/*
 * This convoluted piece of code attempts to get
 * the penril in sync.  If you don't have FIONREAD
 * there are gory ways to simulate this.
 */
static int
pensync(fd)
{
    int already = 0, nread;
    long temp;
    char buf[60];

    /*
     * Toggle DTR to force anyone off that might have left
     * the modem connected, and insure a consistent state
     * to start from.
     *
     * If you don't have the ioctl calls to diddle directly
     * with DTR, you can always try setting the baud rate to 0.
     */
    ioctl(FD, TIOCCDTR, 0);
    sleep(1);
    ioctl(FD, TIOCSDTR, 0);
    while (already < MAXRETRY) {
        /*
         * After reseting the modem, send it two \r's to
         * autobaud on. Make sure to delay between them
         * so the modem can frame the incoming characters.
         */
        write(fd, "\r", 1);
        sleep(2);
        if (ioctl(fd, FIONREAD, &temp) < 0) {
            perror("tip: ioctl");
            continue;
        }
        nread = temp;
        while (nread > 0) {
            read(fd, buf, min(nread, 70));
            if (((buf[nread - 6] & 0177) == 'B') &&
                ((buf[nread - 5] & 0177) == 'P') &&
                ((buf[nread - 4] & 0177) == 'S') &&
                ((buf[nread - 1] & 0177) == '>'))
                return (1);
            nread -= min(nread, 70);
        }
        sleep(1);
        already++;
    }
    return (0);
}
