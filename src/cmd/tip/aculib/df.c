/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Dial the DF02-AC or DF03-AC
 */
#include "tip.h"

static jmp_buf Sjbuf;

static void
timeout(int sig)
{
    longjmp(Sjbuf, 1);
}

void df_disconnect()
{
    int rw = 2;

    write(FD, "\001", 1);
    sleep(1);
    ioctl(FD, TIOCFLUSH, &rw);
}

int df_dialer(
    char *num, char *acu,
    int df03)
{
    register int f = FD;
    struct sgttyb buf;
    int speed = 0, rw = 2;
    char c = '\0';

    ioctl(f, TIOCHPCL, 0);      /* make sure it hangs up when done */
    if (setjmp(Sjbuf)) {
        printf("connection timed out\r\n");
        df_disconnect();
        return (0);
    }
    if (boolean(value(VERBOSE)))
        printf("\ndialing...");
    fflush(stdout);
#ifdef TIOCMSET
    if (df03) {
        int st = TIOCM_ST;  /* secondary Transmit flag */

        ioctl(f, TIOCGETP, &buf);
        if (buf.sg_ospeed != B1200) {   /* must dial at 1200 baud */
            speed = buf.sg_ospeed;
            buf.sg_ospeed = buf.sg_ispeed = B1200;
            ioctl(f, TIOCSETP, &buf);
            ioctl(f, TIOCMBIC, &st); /* clear ST for 300 baud */
        } else
            ioctl(f, TIOCMBIS, &st); /* set ST for 1200 baud */
    }
#endif
    signal(SIGALRM, timeout);
    alarm(5 * strlen(num) + 10);
    ioctl(f, TIOCFLUSH, &rw);
    write(f, "\001", 1);
    sleep(1);
    write(f, "\002", 1);
    write(f, num, strlen(num));
    read(f, &c, 1);
#ifdef TIOCMSET
    if (df03 && speed) {
        buf.sg_ispeed = buf.sg_ospeed = speed;
        ioctl(f, TIOCSETP, &buf);
    }
#endif
    return (c == 'A');
}

int df02_dialer(
    char *num, char *acu)
{
    return (df_dialer(num, acu, 0));
}

int df03_dialer(
    char *num, char *acu)
{
    return (df_dialer(num, acu, 1));
}

void df_abort()
{
    df_disconnect();
}
