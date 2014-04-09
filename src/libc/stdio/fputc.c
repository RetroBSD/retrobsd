#include <stdio.h>

int
fputc (c, fp)
	register int c;
	register FILE *fp;
{
	return putc (c, fp);
}
