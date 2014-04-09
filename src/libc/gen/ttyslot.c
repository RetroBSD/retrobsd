/*
 * Copyright (c) 1984 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Return the number of the slot in the utmp file
 * corresponding to the current user: try for file 0, 1, 2.
 * Definition is the line number in the /etc/ttys file.
 */
#include <ttyent.h>
#include <string.h>
#include <unistd.h>

int
ttyslot()
{
	register struct ttyent *ty;
	register char *tp, *p;
	register int s;

	if (! (tp = ttyname(0)) &&
	    ! (tp = ttyname(1)) &&
	    ! (tp = ttyname(2)))
		return 0;
        p = strrchr(tp, '/');
	if (! p)
		p = tp;
	else
		p++;
	setttyent();
	s = 0;
	while ((ty = getttyent())) {
		s++;
		if (strcmp(ty->ty_name, p) == 0) {
			endttyent();
			return s;
		}
	}
	endttyent();
	return 0;
}
