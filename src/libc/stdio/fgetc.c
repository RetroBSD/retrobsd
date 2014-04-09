#include <stdio.h>

int
fgetc(fp)
        register FILE *fp;
{
	return getc(fp);
}
