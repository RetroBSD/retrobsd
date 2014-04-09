/*
 * Copyright (c) 1983 Regents of the University of California,
 * All rights reserved.  Redistribution permitted subject to
 * the terms of the Berkeley Software License Agreement.
 */
#include "externs.h"

int
card(array, size)		/* for beenthere, injuries */
	register char *array;
	int size;
{
	register char *end = array + size;
	register int i = 0;

	while (array < end)
		if (*array++)
			i++;
	return (i);
}

int
ucard(array)
	register unsigned *array;
{
	register int j = 0, n;

	for (n = 0; n < NUMOFOBJECTS; n++)
		if (testbit(array, n))
			    j++;
	return (j);
}
