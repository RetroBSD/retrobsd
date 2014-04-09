/*
 * Returns the number of
 * non-NULL bytes in string argument.
 */
#include <string.h>

int
strlen(s)
	register char *s;
{
	register n;

	n = 0;
	while (*s++)
		n++;
	return(n);
}
