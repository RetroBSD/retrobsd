/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <string.h>

/*
 * Sys5 compat routine
 */
int
memcmp (const void *vs1, const void *vs2, size_t n)
{
	register const char *s1 = vs1, *s2 = vs2;

	while (n-- > 0)
		if (*s1++ != *s2++)
			return (*--s1 - *--s2);
	return (0);
}
