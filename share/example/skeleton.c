/*
 * Sample program.
 *
 * Copyright (C) 1993-2004 Serge Vakulenko, <vak@cronyx.ru>
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You can redistribute this file and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software Foundation;
 * either version 2 of the License, or (at your discretion) any later version.
 * See the accompanying file "COPYING.txt" for more details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const char version[] = "1.0";
const char copyright[] = "Copyright (C) 1993 Serge Vakulenko";

char *progname;
int verbose;
int trace;
int debug;

extern char *optarg;
extern int optind;

void usage ()
{
	fprintf (stderr, "Skeleton of generic C program, Version %s, %s\n", version, copyright);
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
	int count = 1;

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
