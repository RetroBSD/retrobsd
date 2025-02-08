/*
 * tee-- pipe fitting
 */
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUFSIZ 8192

int openf[20] = { 1 };
int n = 1;
int t = 0;
int aflag;

char in[BUFSIZ];
char out[BUFSIZ];

static void stash(int p);

void putstr(char *s)
{
    while (*s)
        write(2, s++, 1);
}

int main(int argc, char **argv)
{
    int r, w, p;
    struct stat buf;

    while (argc > 1 && argv[1][0] == '-') {
        switch (argv[1][1]) {
        case 'a':
            aflag++;
            break;
        case 'i':
        case 0:
            signal(SIGINT, SIG_IGN);
        }
        argv++;
        argc--;
    }
    fstat(1, &buf);
    t = (buf.st_mode & S_IFMT) == S_IFCHR;
    if (lseek(1, 0L, 1) == -1 && errno == ESPIPE)
        t++;
    while (argc-- > 1) {
        if (aflag) {
            openf[n] = open(argv[1], 1);
            if (openf[n] < 0)
                openf[n] = creat(argv[1], 0666);
            lseek(openf[n++], 0L, 2);
        } else
            openf[n++] = creat(argv[1], 0666);
        if (stat(argv[1], &buf) >= 0) {
            if ((buf.st_mode & S_IFMT) == S_IFCHR)
                t++;
        } else {
            putstr("tee: cannot open ");
            putstr(argv[1]);
            putstr("\n");
            n--;
        }
        argv++;
    }
    r = w = 0;
    for (;;) {
        for (p = 0; p < BUFSIZ;) {
            if (r >= w) {
                if (t > 0 && p > 0)
                    break;
                w = read(0, in, BUFSIZ);
                r = 0;
                if (w <= 0) {
                    stash(p);
                    exit(0);
                }
            }
            out[p++] = in[r++];
        }
        stash(p);
    }
}

void stash(int p)
{
    int k;
    int i;
    int d;
    d = t ? 16 : p;
    for (i = 0; i < p; i += d)
        for (k = 0; k < n; k++)
            write(openf[k], out + i, d < p - i ? d : p - i);
}
