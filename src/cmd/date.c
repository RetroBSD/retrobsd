/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Date - print and set date
 */
#include <sys/param.h>
#include <sys/time.h>
#include <sys/file.h>
#include <errno.h>
#include <syslog.h>
#include <utmp.h>
#include <paths.h>
#include <tzfile.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ATOI2(ar)   (ar[0] - '0') * 10 + (ar[1] - '0'); ar += 2;

static struct timeval   tv;
static int  retval;

static int  dmsize[] =
    { -1, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static struct utmp  wtmp[2] = {
    { "|", "", "", 0 },
    { "{", "", "", 0 }
};

/*
 * gtime --
 *  convert user's time into number of seconds
 */
static
gtime(ap)
    register char   *ap;        /* user argument */
{
    register int    year, month;
    register char   *C;     /* pointer into time argument */
    struct tm   *L;
    int day, hour, mins, secs;

    for (secs = 0, C = ap;*C;++C) {
        if (*C == '.') {        /* seconds provided */
            if (strlen(C) != 3)
                return(1);
            *C = NULL;
            secs = (C[1] - '0') * 10 + (C[2] - '0');
            break;
        }
        if (!isdigit(*C))
            return(-1);
    }

    L = localtime((time_t *)&tv.tv_sec);
    year = L->tm_year;          /* defaults */
    month = L->tm_mon + 1;
    day = L->tm_mday;

    switch ((int)(C - ap)) {        /* length */
        case 10:            /* yymmddhhmm */
            year = ATOI2(ap);
        case 8:             /* mmddhhmm */
            month = ATOI2(ap);
        case 6:             /* ddhhmm */
            day = ATOI2(ap);
        case 4:             /* hhmm */
            hour = ATOI2(ap);
            mins = ATOI2(ap);
            break;
        default:
            return(1);
    }

    if (*ap || month < 1 || month > 12 || day < 1 || day > 31 ||
         mins < 0 || mins > 59 || secs < 0 || secs > 59)
        return(1);
    if (hour == 24) {
        ++day;
        hour = 0;
    }
    else if (hour < 0 || hour > 23)
        return(1);

    tv.tv_sec = 0;
    year += TM_YEAR_BASE;
/* If year < EPOCH_YEAR, assume it's in the next century and
   the system has not yet been patched to move TM_YEAR_BASE up yet */
    if (year < EPOCH_YEAR)
        year += 100;
    if (isleap(year) && month > 2)
        ++tv.tv_sec;
    for (--year;year >= EPOCH_YEAR;--year)
        tv.tv_sec += isleap(year) ? DAYS_PER_LYEAR : DAYS_PER_NYEAR;
    while (--month)
        tv.tv_sec += dmsize[month];
    tv.tv_sec += day - 1;
    tv.tv_sec = HOURS_PER_DAY * tv.tv_sec + hour;
    tv.tv_sec = MINS_PER_HOUR * tv.tv_sec + mins;
    tv.tv_sec = SECS_PER_MIN * tv.tv_sec + secs;
    return(0);
}

main(argc,argv)
    int argc;
    char    **argv;
{
    static char usage[] = "usage: date [-nu] [-d dst] [-t timezone] [yymmddhhmm[.ss]]\n";
    struct timezone tz;
    char    *ap,            /* time string */
        *tzn;           /* time zone */
    int ch,         /* getopts char */
        uflag,          /* do it in GMT */
        nflag,          /* only set time locally */
        wf;         /* wtmp file descriptor */
    char    *username;
    char    do_update = 0;

    nflag = uflag = 0;
    tz.tz_dsttime = tz.tz_minuteswest = 0;
    while ((ch = getopt(argc,argv,"d:nut:")) != EOF)
        switch((char)ch) {
        case 'd':
            tz.tz_dsttime = atoi(optarg) ? 1 : 0;
            do_update = 1;
            break;
        case 'n':
            nflag = 1;
            break;
        case 't':
            tz.tz_minuteswest = atoi(optarg);
            do_update = 1;
            break;
        case 'u':
            uflag = 1;
            break;
        default:
            fputs(usage,stderr);
            exit(1);
        }
    argc -= optind;
    argv += optind;

    if (argc > 1) {
        fputs(usage,stderr);
        exit(1);
    }

    if ((do_update==1) &&
        settimeofday((struct timeval *)NULL,&tz)) {
        perror("settimeofday");
        retval = 1;
        goto display;
    }

    if (gettimeofday(&tv,&tz)) {
        perror("gettimeofday");
        exit(1);
    }

    if (!argc)
        goto display;

    wtmp[0].ut_time = tv.tv_sec;
    if (gtime(*argv)) {
        fputs(usage,stderr);
        retval = 1;
        goto display;
    }

    if (!uflag) {       /* convert to GMT assuming local time */
        tv.tv_sec += (long)tz.tz_minuteswest * SECS_PER_MIN;
                /* now fix up local daylight time */
        if (localtime((time_t *)&tv.tv_sec)->tm_isdst)
            tv.tv_sec -= SECS_PER_HOUR;
    }
    if (nflag || 1 /*!netsettime(tv)*/) {
        if (settimeofday(&tv,(struct timezone *)0)) {
            perror("settimeofday");
            retval = 1;
            goto display;
        }
        if ((wf = open(_PATH_WTMP, O_WRONLY | O_APPEND)) < 0)
            fputs("date: can't write wtmp file.\n",stderr);
        else {
            (void)time((time_t *)&wtmp[1].ut_time);
            /*NOSTRICT*/
            (void)write(wf,(char *)wtmp,sizeof(wtmp));
            (void)close(wf);
        }
    }

    username = getlogin();
    if (!username || *username == '\0') /* single-user or no tty */
        username = "root";
    syslog(LOG_AUTH | LOG_NOTICE,"date set by %s",username);

display:
    if (gettimeofday(&tv,(struct timezone *)0)) {
        perror("gettimeofday");
        exit(1);
    }
    if (uflag) {
        ap = asctime(gmtime((time_t *)&tv.tv_sec));
        tzn = "GMT";
    }
    else {
        struct tm   *tp;

        tp = localtime((time_t *)&tv.tv_sec);
        ap = asctime(tp);
        tzn = tp->tm_zone;
    }
    printf("%.20s%s%s",ap,tzn,ap + 19);
    exit(retval);
}
