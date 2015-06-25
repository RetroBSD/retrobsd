/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdio.h>

#define MAX ' '

char new[MAX], old[MAX];

int main ()
{
    register int i, j;

    old[0] = '\0';
    while (fgets(&new[0], MAX, stdin) != NULL) {
        for (i=0; i<MAX && old[i]==new[i]; i++);
        if (i >= MAX) {
            fprintf(stderr, "long word\n");
            return 1;
        }
        putc(i, stdout);
        for (j=0; (old[j]=new[j]) != '\n'; j++);
        old[j] = '\0';
        fputs(&old[i], stdout);
    }
    return 0;
}
