/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Print working (current) directory
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>

char *getwd();

main()
{
    char pathname[MAXPATHLEN + 1];

    if (getwd(pathname) == NULL) {
        fprintf(stderr, "pwd: %s\n", pathname);
        exit(1);
    }
    printf("%s\n", pathname);
    exit(0);
}
