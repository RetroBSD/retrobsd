/*
 * Copyright (c) 1987 Regents of the University of California.
 * This file may be freely redistributed provided that this
 * notice remains attached.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * getenv(name) --
 *	Returns ptr to value associated with name, if any, else NULL.
 */
char *
getenv(name)
	char *name;
{
	int	offset;
	char	*_findenv();

	return(_findenv(name,&offset));
}

/*
 * _findenv(name,offset) --
 *	Returns pointer to value associated with name, if any, else NULL.
 *	Sets offset to be the offset of the name/value combination in the
 *	environmental array, for use by setenv(3) and unsetenv(3).
 *	Explicitly removes '=' in argument name.
 *
 *	This routine *should* be a static; don't use it.
 */
char *
_findenv(name, offset)
	register const char *name;
	int	*offset;
{
	register int	len;
	register char	**P, *C;
	register const char *E;

        len = 0;
	for (E = name; *E && *E != '='; ++E)
                ++len;
	for (P = environ; *P; ++P)
		if (!strncmp(*P, name, len))
			if (*(C = *P + len) == '=') {
				*offset = P - environ;
				return(++C);
			}
	return(NULL);
}
