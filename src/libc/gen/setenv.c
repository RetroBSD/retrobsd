/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

/*
 * setenv(name,value,rewrite)
 *	Set the value of the environmental variable "name" to be
 *	"value".  If rewrite is set, replace any current value.
 */
int
setenv(name, value, rewrite)
	register const char	*name,
				*value;
	int	rewrite;
{
	static int	alloced;		/* if allocated space before */
	register char	*C;
	register const char *E;
	int l_value, offset;

	if (*value == '=')			/* no `=' in value */
		++value;
	l_value = strlen(value);
	if ((C = _findenv(name,&offset))) {	/* find if already exists */
		if (!rewrite)
			return(0);
		if (strlen(C) >= l_value) {	/* old larger; copy over */
			while ((*C++ = *value++));
			return(0);
		}
	}
	else {					/* create new slot */
		register int	cnt;
		register char	**P;

		for (P = environ,cnt = 0;*P;++P,++cnt);
		if (alloced) {			/* just increase size */
			environ = (char **)realloc((char *)environ,
			    (u_int)(sizeof(char *) * (cnt + 2)));
			if (!environ)
				return(-1);
		}
		else {				/* get new space */
			alloced = 1;		/* copy old entries into it */
			P = (char **)malloc((u_int)(sizeof(char *) *
			    (cnt + 2)));
			if (!P)
				return(-1);
			bcopy(environ,P,cnt * sizeof(char *));
			environ = P;
		}
		environ[cnt + 1] = NULL;
		offset = cnt;
	}
	for (E = name; *E && *E != '='; ++E);	/* no `=' in name */
	if (!(environ[offset] =			/* name + `=' + value */
	    malloc((u_int)((int)(E - name) + l_value + 2))))
		return(-1);
	for (C = environ[offset]; (*C = *name++) && *C != '='; ++C);
	for (*C++ = '='; (*C++ = *value++); );
	return(0);
}

/*
 * unsetenv(name) --
 *	Delete environmental variable "name".
 */
int
unsetenv(name)
	const char	*name;
{
	register char	**P;
	int	offset;

	while (_findenv(name,&offset))		/* if set multiple times */
		for (P = &environ[offset];;++P)
			if (!(*P = *(P + 1)))
				break;
        return 0;
}
