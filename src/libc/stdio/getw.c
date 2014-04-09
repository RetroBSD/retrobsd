#include <stdio.h>

int
getw(iop)
        register FILE *iop;
{
	register int i;
	register char *p;
	int w;

	p = (char *)&w;
	for (i=sizeof(int); --i>=0;)
		*p++ = getc(iop);
	if (feof(iop))
		return(EOF);
	return(w);
}

#ifdef pdp11
long
getlw(iop)
register FILE *iop;
{
	register int i;
	register char *p;
	long w;

	p = (char *)&w;
	for (i=sizeof(long); --i>=0;)
		*p++ = getc(iop);
	if (feof(iop))
		return(EOF);
	return(w);
}
#endif
