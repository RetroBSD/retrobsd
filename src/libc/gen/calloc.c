/*
 * Calloc - allocate and clear memory block
 */
#include <sys/types.h>
#include <stdlib.h>
#include <strings.h>
#include <stdlib.h>

void *
calloc(size_t num, size_t size)
{
	register char *p;

	size *= num;
	p = malloc(size);
	if (p)
		bzero(p, size);
	return (p);
}
