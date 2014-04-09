#ifdef CROSS
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif

void putone(byte)
        register int byte;
{
	register int i;

	putchar('|');
	for (i=7; i>=0; --i) {
		if (i==2)
			putchar('.');
		if (byte & (1 << i))
			putchar('o');
		else
			putchar(' ');
	}
	putchar('|');
	putchar('\n');
}

int main()
{
	register int c;

	printf("___________\n");
	while ((c = getchar()) != EOF)
		putone(c);
	printf("___________\n");
	return 0;
}
