#include <stdio.h>

int
puts(s)
	register const char *s;
{
	register int c;

	while ((c = *s++))
		putchar(c);
	return(putchar('\n'));
}
