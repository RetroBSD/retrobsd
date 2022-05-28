/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 */
#include "defs.h"

char*
setbrk(incr)
{
	register char *a = sbrk(incr);

	brkend = a + incr;
	return(a);
}
