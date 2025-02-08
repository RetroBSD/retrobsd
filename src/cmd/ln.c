/*
 * ln
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

struct stat stb;
int fflag; /* force flag set? */
int sflag;
char name[BUFSIZ];

static int linkit(char *from, char *to);

int main(int argc, char **argv)
{
    int i, r;

    argc--, argv++;
again:
    if (argc && strcmp(argv[0], "-f") == 0) {
        fflag++;
        argv++;
        argc--;
    }
    if (argc && strcmp(argv[0], "-s") == 0) {
        sflag++;
        argv++;
        argc--;
    }
    if (argc == 0)
        goto usage;
    else if (argc == 1) {
        argv[argc] = ".";
        argc++;
    }
    if (sflag == 0 && argc > 2) {
        if (stat(argv[argc - 1], &stb) < 0)
            goto usage;
        if ((stb.st_mode & S_IFMT) != S_IFDIR)
            goto usage;
    }
    r = 0;
    for (i = 0; i < argc - 1; i++)
        r |= linkit(argv[i], argv[argc - 1]);
    exit(r);
usage:
    fprintf(stderr, "Usage: ln [ -s ] f1\nor: ln [ -s ] f1 f2\nln [ -s ] f1 ... fn d2\n");
    exit(1);
}

int linkit(char *from, char *to)
{
    char *tail;
    int (*linkf)(const char *target, const char *linkpath) = sflag ? symlink : link;

    /* is target a directory? */
    if (sflag == 0 && fflag == 0 && stat(from, &stb) >= 0 && (stb.st_mode & S_IFMT) == S_IFDIR) {
        printf("%s is a directory\n", from);
        return (1);
    }
    if (stat(to, &stb) >= 0 && (stb.st_mode & S_IFMT) == S_IFDIR) {
        tail = rindex(from, '/');
        if (tail == 0)
            tail = from;
        else
            tail++;
        sprintf(name, "%s/%s", to, tail);
        to = name;
    }
    if ((*linkf)(from, to) < 0) {
        if (errno == EEXIST || sflag)
            perror(to);
        else if (access(from, 0) < 0)
            perror(from);
        else
            perror(to);
        return (1);
    }
    return (0);
}
