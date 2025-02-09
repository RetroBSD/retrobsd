/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 */
#include "defs.h"

char *setbrk(int incr)
{
    register char *a = sbrk(incr);

    brkend = a + incr;
    return (a);
}
