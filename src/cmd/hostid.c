/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#if HAVE_NET
#include <netdb.h>
#include <arpa/inet.h>
#endif

main(argc, argv)
    int argc;
    char **argv;
{
    register char *id;
    u_long addr;
    long hostid;
    struct hostent *hp;

    if (argc < 2) {
        printf("0x%lx\n", gethostid());
        exit(0);
    }

    id = argv[1];
#if HAVE_NET
    if (hp = gethostbyname(id)) {
        bcopy(hp->h_addr, &addr, sizeof(addr));
        hostid = addr;
    } else
#endif
    if (index(id, '.')) {
        if ((hostid = inet_addr(id)) == -1L)
            usage();
    } else {
        if (id[0] == '0' && (id[1] == 'x' || id[1] == 'X'))
            id += 2;
        if (sscanf(id, "%lx", &hostid) != 1)
            usage();
    }

    if (sethostid(hostid) < 0)
        err(1, "sethostid");
    exit(0);
}

usage()
{
    errx (1,"usage: [hexnum or internet name/address]");
/* NOTREACHED */
}
