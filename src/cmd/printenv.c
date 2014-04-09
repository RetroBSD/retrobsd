/*
 * printenv
 *
 * Bill Joy, UCB
 * February, 1979
 *
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int prefix(cp, dp)
    char *cp, *dp;
{
    while (*cp && *dp && *cp == *dp)
        cp++, dp++;
    if (*cp == 0)
        return (*dp == '=');
    return 0;
}

int main(argc, argv)
    int argc;
    char *argv[];
{
    register char **ep;
    int found = 0;

    if (! environ)
        return 1;

    argc--, argv++;
    for (ep = environ; *ep; ep++) {
        if (argc == 0 || prefix(argv[0], *ep)) {
            register char *cp = *ep;

            found++;
            if (argc) {
                while (*cp && *cp != '=')
                    cp++;
                if (*cp == '=')
                    cp++;
            }
            printf("%s\n", cp);
        }
    }
    return !found;
}
