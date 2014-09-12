/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "robots.h"
#include <stdlib.h>

#define	IS_SAME(p,y,x)	((p).y != -1 && (p).y == y && (p).x == x)

static int
rnd(range)
int	range;
{
	return random() % range;
}

/*
 * rnd_pos:
 *	Pick a random, unoccupied position
 */
COORD *
rnd_pos()
{
	static COORD	pos;
	static int	call = 0;

	do {
		pos.y = rnd(Y_FIELDSIZE - 1) + 1;
		pos.x = rnd(X_FIELDSIZE - 1) + 1;
		refresh();
	} while (Field[pos.y][pos.x] != 0);
	call++;
	return &pos;
}
