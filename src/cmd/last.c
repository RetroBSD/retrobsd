/*
 * last
 *
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <utmp.h>
#include <paths.h>

#define NMAX    sizeof(buf[0].ut_name)
#define LMAX    sizeof(buf[0].ut_line)
#define HMAX    sizeof(buf[0].ut_host)
#define SECDAY  ((long)24*60*60)

#define lineq(a,b)  (!strncmp(a,b,LMAX))
#define nameq(a,b)  (!strncmp(a,b,NMAX))
#define hosteq(a,b) (!strncmp(a,b,HMAX))

#define MAXTTYS 256

char    **argv;
int argc;
int nameargs;

struct  utmp buf[128];
char    ttnames[MAXTTYS][LMAX+1];
long    logouts[MAXTTYS];

char    *ctime(), *strspl();

void onintr(signo)
    int signo;
{
    char *ct;

    if (signo == SIGQUIT)
        signal(SIGQUIT, onintr);
    ct = ctime(&buf[0].ut_time);
    printf("\ninterrupted %10.10s %5.5s \n", ct, ct + 11);
    fflush(stdout);
    if (signo == SIGINT)
        exit(1);
}

void
usage(progname)
    char *progname;
{
    printf("Usage: %s [ -f filename ] [-number] [name...] [tty...]\n",
        progname);
    exit(1);
}

main(ac, av)
    char **av;
{
    register int i, k;
    int wtmp;
    off_t bl;
    char *ct;
    char wtmpfile[256];
    char progname[256];
    register struct utmp *bp;
    long otime;
    struct stat stb;
    int print;
    int sinput = 0;
    char * crmsg = (char *)0;
    long crtime;
    long outrec = 0;
    long maxrec = 0x7fffffffL;

    time(&buf[0].ut_time);
    strcpy(wtmpfile, _PATH_WTMP);
    strcpy(progname,av[0]);
    ac--, av++;
    nameargs = argc = ac;
    argv = av;
    for (i = 0; i < argc; i++) {
        if (argv[i][0] == '-' ) {
            if ( argv[i][1] >= '0' && argv[i][1] <= '9') {
                maxrec = atoi(argv[i]+1);
                nameargs--;
                continue;
            } else {
                if (argv[i][1] == 'f') {
                    i++;
                    if ( i < argc) {
                        strcpy(wtmpfile,argv[i]);
                        nameargs = nameargs -2;
                        continue;
                    } else {
                        usage(progname);
                    }
                } else {
                    usage(progname);
                }
            }
        }
        if (strlen(argv[i])>2)
            continue;
        if (!strcmp(argv[i], "~"))
            continue;
        if (!strcmp(argv[i], "ftp"))
            continue;
        if (!strcmp(argv[i], "uucp"))
            continue;
        if (getpwnam(argv[i]))
            continue;
        argv[i] = strspl("tty", argv[i]);
    }
    wtmp = open(wtmpfile, 0);
    if (wtmp < 0) {
        perror(wtmpfile);
        exit(1);
    }
    fstat(wtmp, &stb);
    bl = (stb.st_size + sizeof (buf)-1) / sizeof (buf);
    if (signal(SIGINT, SIG_IGN) != SIG_IGN) {
        signal(SIGINT, onintr);
        signal(SIGQUIT, onintr);
    }
    for (bl--; bl >= 0; bl--) {
        lseek(wtmp, bl * sizeof (buf), 0);
        bp = &buf[read(wtmp, buf, sizeof (buf)) / sizeof(buf[0]) - 1];
        for ( ; bp >= buf; bp--) {
            print = want(bp);
            if (print) {
                ct = ctime(&bp->ut_time);
                printf("%-*.*s  %-*.*s %-*.*s %10.10s %5.5s ",
                    NMAX, NMAX, bp->ut_name,
                    LMAX, LMAX, bp->ut_line,
                    HMAX, HMAX, bp->ut_host,
                    ct, 11+ct);
            }
            for (i = 0; i < MAXTTYS; i++) {
                if (ttnames[i][0] == 0) {
                    strncpy(ttnames[i], bp->ut_line,
                        sizeof(bp->ut_line));
                    otime = logouts[i];
                    logouts[i] = bp->ut_time;
                    break;
                }
                if (lineq(ttnames[i], bp->ut_line)) {
                    otime = logouts[i];
                    logouts[i] = bp->ut_time;
                    break;
                }
            }
            if (print) {
                if (lineq(bp->ut_line, "~"))
                    printf("\n");
                else if (otime == 0)
                    printf("  still logged in\n");
                else {
                    long delta;
                    if (otime < 0) {
                        otime = -otime;
                        printf("- %s", crmsg);
                    } else
                        printf("- %5.5s",
                            ctime(&otime)+11);
                    delta = otime - bp->ut_time;
                    if (delta < SECDAY)
                        printf("  (%5.5s)\n",
                        asctime(gmtime(&delta))+11);
                    else
                        printf(" (%ld+%5.5s)\n",
                        delta / SECDAY,
                        asctime(gmtime(&delta))+11);
                }
                fflush(stdout);
                if (++outrec >= maxrec)
                    exit(0);
            }
            if (lineq(bp->ut_line, "~")) {
                for (i = 0; i < MAXTTYS; i++)
                    logouts[i] = -bp->ut_time;
                if (nameq(bp->ut_name, "shutdown"))
                    crmsg = "down ";
                else
                    crmsg = "crash";
            }
        }
    }
    ct = ctime(&buf[0].ut_time);
    printf("\nwtmp begins %10.10s %5.5s \n", ct, ct + 11);
    exit(0);
}

want(bp)
    struct utmp *bp;
{
    register char **av;
    register int ac;

    if (bp->ut_line[0] == '~' && bp->ut_name[0] == '\0')
        strcpy(bp->ut_name, "reboot");      /* bandaid */
    if (strncmp(bp->ut_line, "ftp", 3) == 0)
        bp->ut_line[3] = '\0';
    if (strncmp(bp->ut_line, "uucp", 4) == 0)
        bp->ut_line[4] = '\0';
    if (bp->ut_name[0] == 0)
        return (0);
    if (nameargs == 0)
        return (1);
    av = argv;
    for (ac = 0; ac < argc; ac++, av++) {
        if (av[0][0] == '-')
            continue;
        if (nameq(*av, bp->ut_name) || lineq(*av, bp->ut_line))
            return (1);
    }
    return (0);
}

char *
strspl(left, right)
    char *left, *right;
{
    char *res = (char *)malloc(strlen(left)+strlen(right)+1);

    strcpy(res, left);
    strcat(res, right);
    return (res);
}
