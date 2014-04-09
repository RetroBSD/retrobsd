/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdlib.h>

char *
valloc(i)
	int i;
{
#ifdef pdp11
	/*
	 * page boudaries don't mean anything on a PDP-11 and the cost in
	 * memory is just too prohibitive to blindly use the non-PDP-11
	 * algorithm.
	 */
	return(malloc(i));
#else
	int valsiz = getpagesize(), j;
	char *cp = malloc(i + (valsiz-1));

	j = ((int)cp + (valsiz-1)) &~ (valsiz-1);
	return ((char *)j);
#endif
}
