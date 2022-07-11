/*
 * Deal with duplicated lines in a file
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

int fields;
int letters;
int linec;
char mode;
int uniq;

static void printe(char *p, char *s);
static int gline(char buf[]);
static void pline(char buf[]);
static int equal(char b1[], char b2[]);
static char *skip(char *s);

int main(int argc, char *argv[])
{
    static char b1[1000], b2[1000];

    while (argc > 1) {
        if (*argv[1] == '-') {
            if (isdigit(argv[1][1]))
                fields = atoi(&argv[1][1]);
            else
                mode = argv[1][1];
            argc--;
            argv++;
            continue;
        }
        if (*argv[1] == '+') {
            letters = atoi(&argv[1][1]);
            argc--;
            argv++;
            continue;
        }
        if (freopen(argv[1], "r", stdin) == NULL)
            printe("cannot open %s\n", argv[1]);
        break;
    }
    if (argc > 2 && freopen(argv[2], "w", stdout) == NULL)
        printe("cannot create %s\n", argv[2]);

    if (gline(b1))
        exit(0);
    for (;;) {
        linec++;
        if (gline(b2)) {
            pline(b1);
            exit(0);
        }
        if (!equal(b1, b2)) {
            pline(b1);
            linec = 0;
            do {
                linec++;
                if (gline(b1)) {
                    pline(b2);
                    exit(0);
                }
            } while (equal(b1, b2));
            pline(b2);
            linec = 0;
        }
    }
}

int gline(char buf[])
{
    int c;

    while ((c = getchar()) != '\n') {
        if (c == EOF)
            return (1);
        *buf++ = c;
    }
    *buf = 0;
    return (0);
}

void pline(char buf[])
{
    switch (mode) {
    case 'u':
        if (uniq) {
            uniq = 0;
            return;
        }
        break;

    case 'd':
        if (uniq)
            break;
        return;

    case 'c':
        printf("%4d ", linec);
    }
    uniq = 0;
    fputs(buf, stdout);
    putchar('\n');
}

int equal(char b1[], char b2[])
{
    char c;

    b1 = skip(b1);
    b2 = skip(b2);
    while ((c = *b1++) != 0)
        if (c != *b2++)
            return (0);
    if (*b2 != 0)
        return (0);
    uniq++;
    return (1);
}

char *skip(char *s)
{
    int nf, nl;

    nf = nl = 0;
    while (nf++ < fields) {
        while (*s == ' ' || *s == '\t')
            s++;
        while (!(*s == ' ' || *s == '\t' || *s == 0))
            s++;
    }
    while (nl++ < letters && *s != 0)
        s++;
    return (s);
}

void printe(char *p, char *s)
{
    fprintf(stderr, p, s);
    exit(1);
}
