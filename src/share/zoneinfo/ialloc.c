/*
 *	@(#)ialloc.c	1.1 ialloc.c 3/4/87
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zic.h"

#ifndef alloc_t
#define alloc_t unsigned
#endif /* !alloc_t */

#ifdef MAL
#define NULLMAL(x) ((x) == NULL || (x) == MAL)
#else /* !MAL */
#define NULLMAL(x) ((x) == NULL)
#endif /* !MAL */

char *imalloc(int n)
{
#ifdef MAL
    register char *result;

    if (n == 0)
        n = 1;
    result = malloc((alloc_t)n);
    return (result == MAL) ? NULL : result;
#else  /* !MAL */
    if (n == 0)
        n = 1;
    return malloc((alloc_t)n);
#endif /* !MAL */
}

char *icalloc(int nelem, int elsize)
{
    if (nelem == 0 || elsize == 0)
        nelem = elsize = 1;
    return calloc((alloc_t)nelem, (alloc_t)elsize);
}

char *irealloc(char *pointer, int size)
{
    if (NULLMAL(pointer))
        return imalloc(size);
    if (size == 0)
        size = 1;
    return realloc(pointer, (alloc_t)size);
}

char *icatalloc(char *old, char *new)
{
    register char *result;
    register int oldsize, newsize;

    oldsize = NULLMAL(old) ? 0 : strlen(old);
    newsize = NULLMAL(new) ? 0 : strlen(new);
    if ((result = irealloc(old, oldsize + newsize + 1)) != NULL)
        if (!NULLMAL(new))
            (void)strcpy(result + oldsize, new);
    return result;
}

char *icpyalloc(char *string)
{
    return icatalloc((char *)NULL, string);
}

void ifree(char *p)
{
    if (!NULLMAL(p))
        free(p);
}
