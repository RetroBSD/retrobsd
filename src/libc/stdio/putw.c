#include <stdio.h>

int
putw(int w, FILE *iop)
{
	register char *p;
	register int i;

	p = (char *)&w;
	for (i=sizeof(int); --i>=0;)
		putc(*p++, iop);
	return(ferror(iop));
}

#ifdef pdp11
int
putlw(long w, FILE *iop)
{
	register char *p;
	register int i;

	p = (char *)&w;
	for (i=sizeof(long); --i>=0;)
		putc(*p++, iop);
	return(ferror(iop));
}
#endif
