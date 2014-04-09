/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Remove directory
 */
#include <stdio.h>
#include <stdlib.h>

main(argc,argv)
    int argc;
    char **argv;
{
    int errors = 0;

    if (argc < 2) {
        fprintf(stderr, "usage: %s directory ...\n", argv[0]);
        exit(1);
    }
    while (--argc)
        if (rmdir(*++argv) < 0) {
            fprintf(stderr, "rmdir: ");
            perror(*argv);;
            errors++;
        }
    exit(errors != 0);
}
