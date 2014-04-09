/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
# include "trek.h"
# include <stdarg.h>
# include <errno.h>
# include <stdlib.h>

/*
**  ASSORTED UTILITY ROUTINES
*/

/*
**  BLOCK MOVE
**
**	Moves a block of storage of length `l' bytes from the data
**	area pointed to by `a' to the area pointed to by `b'.
**	Returns the address of the byte following the `b' field.
**	Overflow of `b' is not tested.
*/
void *
bmove(a, b, l)
        void	*a, *b;
        int	l;
{
	register int		n;
	register char		*p, *q;

	p = a;
	q = b;
	n = l;
	while (n--)
		*q++ = *p++;
	return (q);
}


/*
**  STRING EQUALITY TEST
**	null-terminated strings `a' and `b' are tested for
**	absolute equality.
**	returns one if equal, zero otherwise.
*/
int
sequal(a, b)
        char	*a, *b;
{
	register char		*p, *q;

	p = a;
	q = b;
	while (*p || *q)
		if (*p++ != *q++)
			return(0);
	return(1);
}


/*
**  FIND STRING LENGTH
**
**	The length of string `s' (excluding the null byte which
**		terminates the string) is returned.
*/
int
length(s)
        char	*s;
{
	register int	l;
	register char	*p;

	l = 0;
	p = s;
	while (*p++)
		l++;
	return(l);
}


/*
**  SYSTEM ERROR
*/
void
syserr(char *fmt, ...)
{
        va_list ap;

	printf("\n\07TREK SYSERR: ");
        va_start(ap, fmt);
	vprintf(fmt, ap);
        va_end(ap);
	printf("\n");
	if (errno)
		printf("\tsystem error %d\n", errno);
	exit(-1);
}
