#include <stdio.h>
#include <stdint.h>

/* Fixes for RetroBSD missing functions */

static const double one = 1.0, Zero[] = {0.0, -0.0,};


int fgetpos(FILE *stream, int *pos)
{
	*pos = ftell(stream);
	return *pos;
}

int fsetpos(FILE *stream, int *pos)
{
	fseek(stream,SEEK_SET,*pos);
	return *pos;
}

