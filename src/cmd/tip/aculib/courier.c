/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#define write cour_write

/*
 * Routines for calling up on a Courier modem.
 * Derived from Hayes driver.
 */
#include "tip.h"

#define MAXRETRY    5

static  int timeout = 0;
static  int connected = 0;
static  jmp_buf timeoutbuf;
static void cour_napx();
void cour_nap(void);
static int coursync();
static int cour_connect();
static int cour_swallow(register char *match);

void cour_disconnect()
{
     /* first hang up the modem*/
    ioctl(FD, TIOCCDTR, 0);
    sleep(1);
    ioctl(FD, TIOCSDTR, 0);
    coursync();             /* reset */
    close(FD);
}

int cour_dialer(
    register char *num,
    char *acu)
{
    register char *cp;
#ifdef ACULOG
    char line[80];
#endif
    if (boolean(value(VERBOSE)))
        printf("Using \"%s\"\n", acu);

    ioctl(FD, TIOCHPCL, 0);
    /*
     * Get in synch.
     */
    if (!coursync()) {
badsynch:
        printf("can't synchronize with courier\n");
#ifdef ACULOG
        logent(value(HOST), num, "courier", "can't synch up");
#endif
        return (0);
    }
    write(FD, "AT E0\r", 6);    /* turn off echoing */
    sleep(1);
#ifdef DEBUG
    if (boolean(value(VERBOSE)))
        verbose_read();
#endif
    ioctl(FD, TIOCFLUSH, 0);    /* flush any clutter */
    write(FD, "AT C1 E0 H0 Q0 X6 V1\r", 21);
    if (!cour_swallow("\r\nOK\r\n"))
        goto badsynch;
    fflush(stdout);
    write(FD, "AT D", 4);
    for (cp = num; *cp; cp++)
        if (*cp == '=')
            *cp = ',';
    write(FD, num, strlen(num));
    write(FD, "\r", 1);
    connected = cour_connect();
#ifdef ACULOG
    if (timeout) {
        sprintf(line, "%d second dial timeout",
            number(value(DIALTIMEOUT)));
        logent(value(HOST), num, "cour", line);
    }
#endif
    if (timeout)
        cour_disconnect();
    return (connected);
}

void cour_abort()
{
    write(FD, "\r", 1); /* send anything to abort the call */
    cour_disconnect();
}

static void
sigALRM(int i)
{
    printf("\07timeout waiting for reply\n");
    timeout = 1;
    longjmp(timeoutbuf, 1);
}

static int
cour_swallow(
    register char *match)
{
    char c;
    sig_t f;

    f = signal(SIGALRM, sigALRM);
    timeout = 0;
    do {
        if (*match =='\0') {
            signal(SIGALRM, f);
            return (1);
        }
        if (setjmp(timeoutbuf)) {
            signal(SIGALRM, f);
            return (0);
        }
        alarm(number(value(DIALTIMEOUT)));
        read(FD, &c, 1);
        alarm(0);
        c &= 0177;
#ifdef DEBUG
        if (boolean(value(VERBOSE)))
            putchar(c);
#endif
    } while (c == *match++);
#ifdef DEBUG
    if (boolean(value(VERBOSE)))
        fflush(stdout);
#endif
    signal(SIGALRM, SIG_DFL);
    return (0);
}

static struct baud_msg {
    char *msg;
    int baud;
} baud_msg[] = {
    { "",       B300    },
    { " 1200",  B1200   },
    { " 2400",  B2400   },
    { 0,        0       },
};

static int
cour_connect()
{
    char c;
    int nc, nl, n;
    struct sgttyb sb;
    char dialer_buf[64];
    struct baud_msg *bm;
    sig_t f;

    if (cour_swallow("\r\n") == 0)
        return (0);
    f = signal(SIGALRM, sigALRM);
again:
    nc = 0; nl = sizeof(dialer_buf)-1;
    bzero(dialer_buf, sizeof(dialer_buf));
    timeout = 0;
    for (nc = 0, nl = sizeof(dialer_buf)-1 ; nl > 0 ; nc++, nl--) {
        if (setjmp(timeoutbuf))
            break;
        alarm(number(value(DIALTIMEOUT)));
        n = read(FD, &c, 1);
        alarm(0);
        if (n <= 0)
            break;
        c &= 0x7f;
        if (c == '\r') {
            if (cour_swallow("\n") == 0)
                break;
            if (!dialer_buf[0])
                goto again;
            if (strcmp(dialer_buf, "RINGING") == 0 &&
                boolean(value(VERBOSE))) {
#ifdef DEBUG
                printf("%s\r\n", dialer_buf);
#endif
                goto again;
            }
            if (strncmp(dialer_buf, "CONNECT",
                    sizeof("CONNECT")-1) != 0)
                break;
            for (bm = baud_msg ; bm ; bm++)
                if (strcmp(bm->msg,
                    dialer_buf+sizeof("CONNECT")-1) == 0) {
                    if (ioctl(FD, TIOCGETP, &sb) < 0) {
                        perror("TIOCGETP");
                        goto error;
                    }
                    sb.sg_ispeed = sb.sg_ospeed = bm->baud;
                    if (ioctl(FD, TIOCSETP, &sb) < 0) {
                        perror("TIOCSETP");
                        goto error;
                    }
                    signal(SIGALRM, f);
#ifdef DEBUG
                    if (boolean(value(VERBOSE)))
                        printf("%s\r\n", dialer_buf);
#endif
                    return (1);
                }
            break;
        }
        dialer_buf[nc] = c;
#ifdef notdef
        if (boolean(value(VERBOSE)))
            putchar(c);
#endif
    }
    printf("%s\r\n", dialer_buf);
error:
    signal(SIGALRM, f);
    return (0);
}

/*
 * This convoluted piece of code attempts to get
 * the courier in sync.
 */
static int
coursync()
{
    int already, len;
    long nread;
    char buf[40];

    for (already=0; already<MAXRETRY; already++) {
        ioctl(FD, TIOCFLUSH, 0);    /* flush any clutter */
        write(FD, "\rAT Z\r", 6);   /* reset modem */
        bzero(buf, sizeof(buf));
        sleep(1);
        ioctl(FD, FIONREAD, &nread);
        if (nread) {
            len = read(FD, buf, sizeof(buf));
            buf[len] = '\0';
#ifdef DEBUG
            printf("coursync: (\"%s\")\n\r", buf);
#endif
            if (strchr(buf, '0') ||
               (strchr(buf, 'O') && strchr(buf, 'K')))
                return(1);
        }
        /*
         * If not strapped for DTR control,
         * try to get command mode.
         */
        sleep(1);
        write(FD, "+++", 3);
        sleep(1);
        /*
         * Toggle DTR to force anyone off that might have left
         * the modem connected.
         */
        ioctl(FD, TIOCCDTR, 0);
        sleep(1);
        ioctl(FD, TIOCSDTR, 0);
    }
    write(FD, "\rAT Z\r", 6);
    return (0);
}

#undef write

extern ssize_t write(int fd, const void *cp, size_t n);

ssize_t cour_write(
    int fd,
    const void *cp,
    size_t n)
{
    struct sgttyb sb;
    int n0 = n;
#ifdef notdef
    if (boolean(value(VERBOSE)))
        write(1, cp, n);
#endif
    ioctl(fd, TIOCGETP, &sb);
    ioctl(fd, TIOCSETP, &sb);
    cour_nap();
    for ( ; n-- ; cp++) {
        write(fd, cp, 1);
        ioctl(fd, TIOCGETP, &sb);
        ioctl(fd, TIOCSETP, &sb);
        cour_nap();
    }
    return n0;
}

#ifdef DEBUG
verbose_read()
{
    long n = 0;
    char buf[BUFSIZ];

    if (ioctl(FD, FIONREAD, &n) < 0)
        return;
    if (n <= 0)
        return;
    if (read(FD, buf, (int)n) != n)
        return;
    write(1, buf, (int)n);
}
#endif

/*
 * Code stolen from /usr/src/lib/libc/gen/sleep.c
 */
#include <sys/time.h>

#define mask(s) sigmask(s)
#define setvec(vec, a) \
    vec.sv_handler = a; vec.sv_mask = vec.sv_onstack = 0

static int napms = 50; /* Give the courier 50 milliseconds between characters */

static int ringring;

void cour_nap()
{
    long omask;
    struct itimerval itv, oitv;
    register struct itimerval *itp = &itv;
    struct sigvec vec, ovec;

    timerclear(&itp->it_interval);
    timerclear(&itp->it_value);
    if (setitimer(ITIMER_REAL, itp, &oitv) < 0)
        return;
    setvec(ovec, SIG_DFL);
    omask = sigblock(mask(SIGALRM));
    itp->it_value.tv_sec = napms/1000;
    itp->it_value.tv_usec = ((napms%1000)*1000);
    setvec(vec, cour_napx);
    ringring = 0;
    (void) sigvec(SIGALRM, &vec, &ovec);
    (void) setitimer(ITIMER_REAL, itp, (struct itimerval *)0);
    while (!ringring)
        sigpause(omask &~ mask(SIGALRM));
    (void) sigvec(SIGALRM, &ovec, (struct sigvec *)0);
    (void) setitimer(ITIMER_REAL, &oitv, (struct itimerval *)0);
    (void) sigsetmask(omask);
}

static void
cour_napx()
{
    ringring = 1;
}
