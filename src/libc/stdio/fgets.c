#include <stdio.h>

char *
fgets(s, n, iop)
        char *s;
        int n;
        register FILE *iop;
{
	register int c = EOF;
	register char *cs;

	cs = s;
	while (--n>0 && (c = getc(iop)) != EOF) {
		*cs++ = c;
		if (c=='\n')
			break;
	}
	if (c == EOF && cs==s)
		return(NULL);
	*cs++ = '\0';
	return(s);
}
