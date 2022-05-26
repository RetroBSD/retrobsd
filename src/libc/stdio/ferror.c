/*
 * A subroutine version of the macro ferror
 */
#define	USE_STDIO_MACROS
#include <stdio.h>

#undef ferror

int
ferror(fp)
        register FILE *fp;
{
	return (fp->_flag & _IOERR) != 0;
}
