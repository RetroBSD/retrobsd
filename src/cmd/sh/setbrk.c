/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 */
#include "defs.h"

char 	*sbrk();

char*
setbrk(incr)
{

	register char *a = sbrk(incr);

	brkend = a + incr;
	return(a);
}
