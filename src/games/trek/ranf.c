/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
# include	<stdlib.h>
# include	"trek.h"

int
ranf(max)
        int	max;
{
	register int	t;

	if (max <= 0)
		return (0);
	t = rand() >> 5;
	return (t % max);
}

double
franf()
{
	double		t;
	t = rand() & 077777;
	return (t / 32767.0);
}
