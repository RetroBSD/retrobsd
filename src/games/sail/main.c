/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "externs.h"

/*ARGSUSED*/
int
main(argc, argv)
	int argc;
	register char **argv;
{
	register char *p;
	int i;
	char stdobuf[BUFSIZ];

	setbuf(stdout, stdobuf);
	(void) srand(getpid());
	issetuid = getuid() != geteuid();
	p = strrchr(*argv, '/');
	if (p)
		p++;
	else
		p = *argv;
	if (strcmp(p, "driver") == 0 || strcmp(p, "saildriver") == 0)
		mode = MODE_DRIVER;
	else if (strcmp(p, "sail.log") == 0)
		mode = MODE_LOGGER;
	else
		mode = MODE_PLAYER;
	while ((p = *++argv) && *p == '-')
		switch (p[1]) {
		case 'd':
			mode = MODE_DRIVER;
			break;
		case 's':
			mode = MODE_LOGGER;
			break;
		case 'D':
			debug++;
			break;
		case 'x':
			randomize++;
			break;
		case 'l':
			longfmt++;
			break;
		case 'b':
			nobells++;
			break;
		default:
			fprintf(stderr, "SAIL: Unknown flag %s.\n", p);
			exit(1);
		}
	if (*argv)
		game = atoi(*argv);
	else
		game = -1;
        i = setjmp(restart);
	if (i)
		mode = i;
	switch (mode) {
	case MODE_PLAYER:
		return pl_main();
	case MODE_DRIVER:
		return dr_main();
	case MODE_LOGGER:
		return lo_main();
	default:
		fprintf(stderr, "SAIL: Unknown mode %d.\n", mode);
		abort();
	}
	/*NOTREACHED*/
}

/*
 * These used to be macros in machdep.h.  The macros were wrong (didn't use
 * sigmask() and thus only computed 16 bit signal masks).  The signal handling
 * in 2.11BSD is now that of 4.4BSD and the macros were fixed (i.e. rewritten)
 * and made into routines to avoid the plethora of inline 'long' operations.
 */
void
blockalarm()
{
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);

	(void)sigprocmask(SIG_BLOCK, &set, NULL);
}

void
unblockalarm()
{
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	(void)sigprocmask(SIG_UNBLOCK, &set, NULL);
}
