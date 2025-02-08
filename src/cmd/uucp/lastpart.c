#include <strings.h>
#include "uucp.h"

/*LINTLIBRARY*/

/*
 *	find last part of file name
 *
 *	return - pointer to last part
 */
char *
lastpart(file)
register char *file;
{
	register char *c;

	c = rindex(file, '/');
	if (c++)
		return c;
	else
		return file;
}
