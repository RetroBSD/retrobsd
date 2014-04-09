/*
 * A subroutine version of the macro fileno
 */
#define	USE_STDIO_MACROS
#include <stdio.h>

#undef fileno

int
fileno(fp)
        register FILE *fp;
{
	return fileno(fp);
}
