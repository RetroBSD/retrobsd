/*
 * A subroutine version of the macro putchar
 */
#define	USE_STDIO_MACROS
#include <stdio.h>

#undef putchar

int
putchar(c)
        register int c;
{
	return putc(c, stdout);
}
