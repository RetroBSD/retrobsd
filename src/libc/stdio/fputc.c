#include <stdio.h>

int
fputc (int c, FILE *fp)
{
	return putc (c, fp);
}
