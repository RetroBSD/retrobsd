/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <string.h>

/*
 * Sys5 compat routine
 */
void *
memset (void *vs, int c, size_t n)
{
	register char *s = vs;

	while (n-- > 0)
		*s++ = c;

	return vs;
}
