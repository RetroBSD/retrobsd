#include <stdio.h>
#include <stdlib.h>

#define LB 256

int one;
int two;
int three;

char *ldr[3];

FILE *ib1;
FILE *ib2;

static FILE *openfil(char *s);
static int rd(FILE *file, char *buf);
static void wr(char *str, int n);
static void copy(FILE *ibuf, char *lbuf, int n);
static int compare(char *a, char *b);

int main(int argc, char *argv[])
{
    int l;
    char lb1[LB], lb2[LB];

    ldr[0] = "";
    ldr[1] = "\t";
    ldr[2] = "\t\t";
    if (argc > 1) {
        if (*argv[1] == '-' && argv[1][1] != 0) {
            l = 1;
            while (*++argv[1]) {
                switch (*argv[1]) {
                case '1':
                    if (!one) {
                        one = 1;
                        ldr[1][0] = ldr[2][l--] = '\0';
                    }
                    break;
                case '2':
                    if (!two) {
                        two = 1;
                        ldr[2][l--] = '\0';
                    }
                    break;
                case '3':
                    three = 1;
                    break;
                default:
                    fprintf(stderr, "comm: illegal flag\n");
                    exit(1);
                }
            }
            argv++;
            argc--;
        }
    }

    if (argc < 3) {
        fprintf(stderr, "comm: arg count\n");
        exit(1);
    }

    ib1 = openfil(argv[1]);
    ib2 = openfil(argv[2]);

    if (rd(ib1, lb1) < 0) {
        if (rd(ib2, lb2) < 0)
            exit(0);
        copy(ib2, lb2, 2);
    }
    if (rd(ib2, lb2) < 0)
        copy(ib1, lb1, 1);

    while (1) {
        switch (compare(lb1, lb2)) {
        case 0:
            wr(lb1, 3);
            if (rd(ib1, lb1) < 0) {
                if (rd(ib2, lb2) < 0)
                    exit(0);
                copy(ib2, lb2, 2);
            }
            if (rd(ib2, lb2) < 0)
                copy(ib1, lb1, 1);
            continue;

        case 1:
            wr(lb1, 1);
            if (rd(ib1, lb1) < 0)
                copy(ib2, lb2, 2);
            continue;

        case 2:
            wr(lb2, 2);
            if (rd(ib2, lb2) < 0)
                copy(ib1, lb1, 1);
            continue;
        }
    }
}

int rd(FILE *file, char *buf)
{
    int i, c;

    i = 0;
    while ((c = getc(file)) != EOF) {
        *buf = c;
        if (c == '\n' || i > LB - 2) {
            *buf = '\0';
            return (0);
        }
        i++;
        buf++;
    }
    return (-1);
}

void wr(char *str, int n)
{
    switch (n) {
    case 1:
        if (one)
            return;
        break;

    case 2:
        if (two)
            return;
        break;

    case 3:
        if (three)
            return;
    }
    printf("%s%s\n", ldr[n - 1], str);
}

void copy(FILE *ibuf, char *lbuf, int n)
{
    do {
        wr(lbuf, n);
    } while (rd(ibuf, lbuf) >= 0);

    exit(0);
}

int compare(char *a, char *b)
{
    char *ra, *rb;

    ra = --a;
    rb = --b;
    while (*++ra == *++rb)
        if (*ra == '\0')
            return (0);
    if (*ra < *rb)
        return (1);
    return (2);
}

FILE *openfil(char *s)
{
    FILE *b;
    if (s[0] == '-' && s[1] == 0)
        b = stdin;
    else if ((b = fopen(s, "r")) == NULL) {
        perror(s);
        exit(1);
    }
    return (b);
}
