/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 */
#include "defs.h"

/* ========     general purpose string handling ======== */

char *
movstr(a, b)
register char   *a, *b;
{
	while (*b++ = *a++);
	return(--b);
}

any(c, s)
register char   c;
char    *s;
{
	register char d;

	while (d = *s++)
	{
		if (d == c)
			return(TRUE);
	}
	return(FALSE);
}

cf(s1, s2)
register char *s1, *s2;
{
	while (*s1++ == *s2)
		if (*s2++ == '\0')
			return(0);
	return *--s1 - *s2;
}

length(as)
char    *as;
{
	register char   *s;

	if (s = as)
		while (*s++);
	return(s - as);
}

char *
movstrn(a, b, n)
	register char *a, *b;
	register int n;
{
	while ((n-- > 0) && *a)
		*b++ = *a++;

	return(b);
}
