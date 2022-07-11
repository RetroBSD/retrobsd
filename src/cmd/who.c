/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <ctype.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/param.h> /* for MAXHOSTNAMELEN */
#include <time.h>
#include <unistd.h>
#include <utmp.h>

#define NMAX sizeof(utmp.ut_name)
#define LMAX sizeof(utmp.ut_line)
#define HMAX sizeof(utmp.ut_host)

struct utmp utmp;
struct passwd *pw;
char hostname[MAXHOSTNAMELEN];

static void putline(void);

int main(int argc, char **argv)
{
    char *tp, *s;
    FILE *fi;

    s = _PATH_UTMP;
    if (argc == 2)
        s = argv[1];
    if (argc == 3) {
        tp = ttyname(0);
        if (tp)
            tp = rindex(tp, '/') + 1;
        else { /* no tty - use best guess from passwd file */
            pw = getpwuid(getuid());
            strncpy(utmp.ut_name, pw ? pw->pw_name : "?", NMAX);
            strcpy(utmp.ut_line, "tty??");
            time(&utmp.ut_time);
            putline();
            exit(0);
        }
    }
    if ((fi = fopen(s, "r")) == NULL) {
        fprintf(stderr, "who: cannot open %s\n", s);
        exit(1);
    }
    while (fread((char *)&utmp, sizeof(utmp), 1, fi) == 1) {
        if (argc == 3) {
            gethostname(hostname, sizeof(hostname));
            if (strcmp(utmp.ut_line, tp))
                continue;
            printf("%s!", hostname);
            putline();
            exit(0);
        }
        if (utmp.ut_name[0] == '\0' && argc == 1)
            continue;
        putline();
    }
}

void putline()
{
    char *cbuf;

    printf("%-*.*s %-*.*s", NMAX, NMAX, utmp.ut_name, LMAX, LMAX, utmp.ut_line);
    cbuf = ctime(&utmp.ut_time);
    printf("%.12s", cbuf + 4);
    if (utmp.ut_host[0])
        printf("\t(%.*s)", HMAX, utmp.ut_host);
    putchar('\n');
}
