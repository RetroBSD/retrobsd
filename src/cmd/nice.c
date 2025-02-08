/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

int main(int argc, char *argv[])
{
    int nicarg = 10;

    if (argc > 1 && argv[1][0] == '-') {
        nicarg = atoi(&argv[1][1]);
        argc--, argv++;
    }
    if (argc < 2) {
        fputs("usage: nice [ -n ] command\n", stderr);
        exit(1);
    }
    if (setpriority(PRIO_PROCESS, 0, getpriority(PRIO_PROCESS, 0) + nicarg) < 0) {
        perror("setpriority");
        exit(1);
    }
    execvp(argv[1], &argv[1]);
    perror(argv[1]);
    exit(1);
}
