/*
 * A subroutine version of the macro fileno
 */
#define	USE_STDIO_MACROS
#include <stdio.h>

#undef fileno

int
fileno(FILE *fp)
{
	return fp->_file;
}
