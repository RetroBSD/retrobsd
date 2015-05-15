/*
 * Generic skeleton for a C program.
 * When you create your own program based on this skeleton,
 * you can replace the author's name and copyright with
 * whatever your want.  When you redistribute this skeleton or
 * enhance it, please leave my name and copyright on it.
 *
 * Copyright (C) 1993-2014 Serge Vakulenko, <vak@cronyx.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const char version[] = "1.0";
const char copyright[] = "Copyright (C) 1993-2014 Serge Vakulenko";

char *progname;         /* Name of the current program (argv[0]) */
int verbose;            /* Option -v */
int trace;              /* Option -t */
int debug;              /* Option -d */

void usage ()
{
    fprintf (stderr, "Generic C skeleton, Version %s, %s\n", version, copyright);
    fprintf (stderr, "Usage:\n\t%s [-vtd] [-r count] file...\n", progname);
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "\t-v\tverbose mode\n");
    fprintf (stderr, "\t-t\ttrace mode\n");
    fprintf (stderr, "\t-d\tdebug\n");
    fprintf (stderr, "\t-r #\trepeat count\n");
    exit (-1);
}

int main (int argc, char **argv)
{
    int count = 1;      /* Option -r # */

    progname = *argv;
    for (;;) {
        switch (getopt (argc, argv, "vtdr:")) {
        case EOF:
            break;
        case 'v':
            ++verbose;
            continue;
        case 't':
            ++trace;
            continue;
        case 'd':
            ++debug;
            continue;
        case 'r':
            count = strtol (optarg, 0, 0);
            continue;
        default:
            usage ();
        }
        break;
    }
    argc -= optind;
    argv += optind;

    if (argc < 1)
        usage ();

    while (count-- > 0) {
        int i;

        for (i=0; i<argc; ++i)
            printf ("%s ", argv[i]);
        printf ("\n");
    }
    return (0);
}
