/*
**  RMAIL -- UUCP mail server.
**
**  This program reads the >From ... remote from ... lines that
**  UUCP is so fond of and turns them into something reasonable.
**  It calls sendmail giving it a -f option built from these
**  lines.
*/
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sysexits.h>

typedef char bool;
#define TRUE 1
#define FALSE 0

bool Debug;

int main(int argc, char **argv)
{
    FILE *out;       /* output to sendmail */
    char lbuf[1024]; /* one line of the message */
    char from[512];  /* accumulated path of sender */
    char ufrom[512]; /* user on remote system */
    char sys[512];   /* a system in path */
    char junk[1024]; /* scratchpad */
    char cmd[2000];
    char *cp;
    char *uf = ufrom; /* ptr into ufrom */
    int i;

#ifdef DEBUG
    if (argc > 1 && strcmp(argv[1], "-T") == 0) {
        Debug = TRUE;
        argc--;
        argv++;
    }
#endif
    if (argc < 2) {
        fprintf(stderr, "Usage: rmail user ...\n");
        exit(EX_USAGE);
    }

    (void)strcpy(from, "");
    (void)strcpy(ufrom, "/dev/null");

    for (;;) {
        (void)fgets(lbuf, sizeof lbuf, stdin);
        if (strncmp(lbuf, "From ", 5) != 0 && strncmp(lbuf, ">From ", 6) != 0)
            break;
        (void)sscanf(lbuf, "%s %s", junk, ufrom);
        cp = lbuf;
        for (;;) {
            cp = index(cp + 1, 'r');
            if (cp == NULL) {
                char *p = rindex(uf, '!');

                if (p != NULL) {
                    *p = '\0';
                    (void)strcpy(sys, uf);
                    uf = p + 1;
                    break;
                }
                cp = "remote from somewhere";
            }
#ifdef DEBUG
            if (Debug)
                printf("cp='%s'\n", cp);
#endif
            if (strncmp(cp, "remote from ", 12) == 0)
                break;
        }
        if (cp != NULL)
            (void)sscanf(cp, "remote from %s", sys);
        (void)strcat(from, sys);
        (void)strcat(from, "!");
#ifdef DEBUG
        if (Debug)
            printf("ufrom='%s', sys='%s', from now '%s'\n", uf, sys, from);
#endif
    }
    (void)strcat(from, uf);

    (void)sprintf(cmd, "%s -ee -f%s -i", _PATH_SENDMAIL, from);
    while (*++argv != NULL) {
        (void)strcat(cmd, " '");
        if (**argv == '(')
            (void)strncat(cmd, *argv + 1, strlen(*argv) - 2);
        else
            (void)strcat(cmd, *argv);
        (void)strcat(cmd, "'");
    }
#ifdef DEBUG
    if (Debug)
        printf("cmd='%s'\n", cmd);
#endif
    out = popen(cmd, "w");
    fputs(lbuf, out);
    while (fgets(lbuf, sizeof lbuf, stdin))
        fputs(lbuf, out);
    i = pclose(out);
    if ((i & 0377) != 0) {
        fprintf(stderr, "pclose: status 0%o\n", i);
        exit(EX_OSERR);
    }

    exit((i >> 8) & 0377);
}
