/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	defined(DOSCCS) && !defined(lint)
static char sccsid[] = "@(#)busy.c	5.1.2 (2.11BSD GTE) 1/16/95";
#endif

/*
 * busy: print an indication of how busy the system is for games.
 */
#ifndef MAX
# define MAX 30
#endif

#include <stdio.h>
main(argc, argv)
char **argv;
{
	double la[3];
	double max;

	getloadavg(la, 3);
	max = la[0];
	if (la[1] > max) max = la[1];
	if (la[2] > max) max = la[2];
	if (argc > 1)
		printf("1=%g, 5=%g, 15=%g, max=%g\n", la[0], la[1], la[2], max);
	if (max > MAX)
		printf("100\n");	/* incredibly high, no games allowed */
	else
		printf("0\n");
	exit(0);
}
