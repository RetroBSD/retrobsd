#include <stdio.h>

int
puts(const char *s)
{
	register int c;

	while ((c = *s++))
		putchar(c);
	return(putchar('\n'));
}
