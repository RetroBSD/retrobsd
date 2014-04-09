/*
 * A subroutine version of the macro feof
 */
#define	USE_STDIO_MACROS
#include <stdio.h>

#undef feof

int
feof(fp)
        register FILE *fp;
{
	return feof(fp);
}
