/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/dir.h>

static char *bindirs[] = {
    // clang-format off
    "/etc",
    "/bin",
    "/sbin",
    "/games",
    "/libexec",
    "/local",
    "/local/bin",
    "/new",
    0
    // clang-format on
};
static char *mandirs[] = {
    // clang-format off
    "/man/man1",
    "/man/man2",
    "/man/man3",
    "/man/man4",
    "/man/man5",
    "/man/man6",
    "/man/man7",
    "/man/man8",
    "/man/manl",
    "/man/mann",
    "/man/mano",
    0
    // clang-format on
};
static char *srcdirs[] = {
    // clang-format off
    "/src/bin",
    "/src/sbin",
    "/src/etc",
    "/src/games",
    "/src/lib",
    "/src/libexec",
    "/src/local",
    "/src/new",
    "/src/include",
    "/src/lib/libc/gen",
    "/src/lib/libc/stdio",
    "/src/lib/libc/sys",
    "/src/lib/libc/net/inet",
    "/src/lib/libc/net/misc",
    0
    // clang-format on
};

char sflag = 1;
char bflag = 1;
char mflag = 1;
char **Sflag;
int Scnt;
char **Bflag;
int Bcnt;
char **Mflag;
int Mcnt;
char uflag;

static void getlist(int *argcp, char ***argvp, char ***flagp, int *cntp);
static void zerof(void);
static void lookup(char *cp);
static void looksrc(char *cp);
static void lookbin(char *cp);
static void lookman(char *cp);
static void find(char **dirs, char *cp);
static void findv(char **dirv, int dirc, char *cp);
static void findin(char *dir, char *cp);
static int itsit(char *cp, char *dp);

/*
 * whereis name
 * look for source, documentation and binaries
 */
int main(int argc, char *argv[])
{
    argc--, argv++;
    if (argc == 0) {
usage:
        fprintf(stderr, "whereis [ -sbmu ] [ -SBM dir ... -f ] name...\n");
        exit(1);
    }
    do {
        if (argv[0][0] == '-') {
            char *cp = argv[0] + 1;

            while (*cp) {
                switch (*cp++) {
                case 'f':
                    break;

                case 'S':
                    getlist(&argc, &argv, &Sflag, &Scnt);
                    break;

                case 'B':
                    getlist(&argc, &argv, &Bflag, &Bcnt);
                    break;

                case 'M':
                    getlist(&argc, &argv, &Mflag, &Mcnt);
                    break;

                case 's':
                    zerof();
                    sflag++;
                    continue;

                case 'u':
                    uflag++;
                    continue;

                case 'b':
                    zerof();
                    bflag++;
                    continue;

                case 'm':
                    zerof();
                    mflag++;
                    continue;

                default:
                    goto usage;
                }
            }
            argv++;
        } else {
            lookup(*argv++);
        }
    } while (--argc > 0);
}

void getlist(int *argcp, char ***argvp, char ***flagp, int *cntp)
{
    (*argvp)++;
    *flagp = *argvp;
    *cntp = 0;
    for ((*argcp)--; *argcp > 0 && (*argvp)[0][0] != '-'; (*argcp)--)
        (*cntp)++, (*argvp)++;
    (*argcp)++;
    (*argvp)--;
}

void zerof()
{
    if (sflag && bflag && mflag)
        sflag = bflag = mflag = 0;
}

int count;
int print;

void lookup(char *cp)
{
    char *dp;

    for (dp = cp; *dp; dp++)
        continue;
    for (; dp > cp; dp--) {
        if (*dp == '.') {
            *dp = 0;
            break;
        }
    }
    for (dp = cp; *dp; dp++)
        if (*dp == '/')
            cp = dp + 1;
    if (uflag) {
        print = 0;
        count = 0;
    } else
        print = 1;
again:
    if (print)
        printf("%s:", cp);
    if (sflag) {
        looksrc(cp);
        if (uflag && print == 0 && count != 1) {
            print = 1;
            goto again;
        }
    }
    count = 0;
    if (bflag) {
        lookbin(cp);
        if (uflag && print == 0 && count != 1) {
            print = 1;
            goto again;
        }
    }
    count = 0;
    if (mflag) {
        lookman(cp);
        if (uflag && print == 0 && count != 1) {
            print = 1;
            goto again;
        }
    }
    if (print)
        printf("\n");
}

void looksrc(char *cp)
{
    if (Sflag == 0) {
        find(srcdirs, cp);
    } else
        findv(Sflag, Scnt, cp);
}

void lookbin(char *cp)
{
    if (Bflag == 0)
        find(bindirs, cp);
    else
        findv(Bflag, Bcnt, cp);
}

void lookman(char *cp)
{
    if (Mflag == 0) {
        find(mandirs, cp);
    } else
        findv(Mflag, Mcnt, cp);
}

void findv(char **dirv, int dirc, char *cp)
{
    while (dirc > 0)
        findin(*dirv++, cp), dirc--;
}

void find(char **dirs, char *cp)
{
    while (*dirs)
        findin(*dirs++, cp);
}

void findin(char *dir, char *cp)
{
    DIR *dirp;
    struct direct *dp;

    dirp = opendir(dir);
    if (dirp == NULL)
        return;
    while ((dp = readdir(dirp)) != NULL) {
        if (itsit(cp, dp->d_name)) {
            count++;
            if (print)
                printf(" %s/%s", dir, dp->d_name);
        }
    }
    closedir(dirp);
}

int itsit(char *cp, char *dp)
{
    int i = strlen(dp);

    if (dp[0] == 's' && dp[1] == '.' && itsit(cp, dp + 2))
        return (1);
    while (*cp && *dp && *cp == *dp)
        cp++, dp++, i--;
    if (*cp == 0 && *dp == 0)
        return (1);
    while (isdigit(*dp))
        dp++;
    if (*cp == 0 && *dp++ == '.') {
        --i;
        while (i > 0 && *dp)
            if (--i, *dp++ == '.')
                return (*dp++ == 'C' && *dp++ == 0);
        return (1);
    }
    return (0);
}
