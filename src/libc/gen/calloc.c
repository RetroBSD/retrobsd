/*
 * Calloc - allocate and clear memory block
 */
#include <sys/types.h>
#include <stdlib.h>
#include <strings.h>

void *
calloc(num, size)
	size_t num, size;
{
	register char *p;

	size *= num;
	p = malloc(size);
	if (p)
		bzero(p, size);
	return (p);
}

void
cfree(p, num, size)
	char *p;
	unsigned num;
	unsigned size;
{
	free(p);
}
