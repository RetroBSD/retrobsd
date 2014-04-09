/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <ttyent.h>
#include <string.h>

struct ttyent *
getttynam(tty)
	const char *tty;
{
	register struct ttyent *t;

	setttyent();
	while ((t = getttyent())) {
		if (strcmp(tty, t->ty_name) == 0)
			break;
	}
	endttyent();
	return (t);
}
