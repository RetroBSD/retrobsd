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
memcpy (vt, vf, n)
	void *vt;
	const void *vf;
	register size_t n;
{
	register char *t = vt;
	register const char *f = vf;

	while (--n >= 0)
		*t++ = *f++;

	return vt;
}
