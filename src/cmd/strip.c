/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifdef CROSS
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif
#include <a.out.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

struct  exec head;
int status;

void
strip(name)
    char *name;
{
    register int f;
    long size;

    f = open(name, O_RDWR);
    if (f < 0) {
        fprintf(stderr, "strip: "); perror(name);
        status = 1;
        goto out;
    }
    if (read(f, (char *)&head, sizeof (head)) < 0 || N_BADMAG(head)) {
        printf("strip: %s not in a.out format\n", name);
        status = 1;
        goto out;
    }
    if (head.a_syms == 0 && head.a_magic != RMAGIC)
        goto out;

    size = N_DATOFF(head) + head.a_data;
    if (ftruncate(f, size) < 0) {
        fprintf(stderr, "strip: ");
        perror(name);
        status = 1;
        goto out;
    }
    head.a_magic = OMAGIC;
    head.a_reltext = 0;
    head.a_reldata = 0;
    head.a_syms = 0;
    (void) lseek(f, (off_t)0, SEEK_SET);
    (void) write(f, (char *)&head, sizeof (head));
out:
    close(f);
}

int
main(argc, argv)
    char *argv[];
{
    register int i;

    while ((i = getopt(argc, argv, "h")) != EOF) {
        switch(i) {
        case 'h':
        default:
usage:                  fprintf(stderr, "Usage:\n");
            fprintf(stderr, "  strip file...\n");
            return(1);
        }
    }
    argc -= optind;
    argv += optind;
    if (argc == 0)
        goto usage;

    signal(SIGHUP, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    for (i = 0; i < argc; i++) {
        strip(argv[i]);
        if (status > 1)
            break;
    }
    return(status);
}
