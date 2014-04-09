/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 */
#include "defs.h"
#include <sys/param.h>

extern char *getenv();

#define BUFLEN  256

int hz = (-1);

static char     buffer[BUFLEN];
static int      index = 0;
char            numbuf[12];

extern void     prc_buff();
extern void     prs_buff();
extern void     prn_buff();
extern void     prs_cntl();
extern void     prn_buff();

/*
 * printing and io conversion
 */
prp()
{
	if ((flags & prompt) == 0 && cmdadr)
	{
		prs_cntl(cmdadr);
		prs(colon);
	}
}

prs(as)
char    *as;
{
	register char   *s;

	if (s = as)
		write(output, s, length(s) - 1);
}

/* print a prompt */
/* it's a subject for future expansion @@@ */
prprompt(as)
char *as;
{
	prs(as);
}

prc(c)
char    c;
{
	if (c)
		write(output, &c, 1);
}

prt(t)
long    t;
{
	register int hr, min, sec;
	char *s;

	if( hz < 0 ){
		s = getenv( "HZ" );
		if( s ) hz = atoi( s );
		else    hz = HZ;
	}

	t += hz / 2;
	t /= hz;
	sec = t % hz;
	t /= HZ;
	min = t % hz;

	if (hr = t / hz)
	{
		prn_buff(hr);
		prc_buff('h');
	}

	prn_buff(min);
	prc_buff('m');
	prn_buff(sec);
	prc_buff('s');
}

prn(n)
	int     n;
{
	itos(n);

	prs(numbuf);
}

itos(n)
{
	register char *abuf;
	register unsigned a, i;
	int pr, d;

	abuf = numbuf;

	pr = FALSE;
	a = n;
	for (i = 10000; i != 1; i /= 10)
	{
		if ((pr |= (d = a / i)))
			*abuf++ = d + '0';
		a %= i;
	}
	*abuf++ = a + '0';
	*abuf++ = 0;
}

stoi(icp)
char    *icp;
{
	register char   *cp = icp;
	register int    r = 0;
	register char   c;

	while ((c = *cp, digit(c)) && c && r >= 0)
	{
		r = r * 10 + c - '0';
		cp++;
	}
	if (r < 0 || cp == icp)
		failed(icp, badnum);
	else
		return(r);
}

prl(n)
long n;
{
	int i;

	i = 11;
	while (n > 0 && --i >= 0)
	{
		numbuf[i] = n % 10 + '0';
		n /= 10;
	}
	numbuf[11] = '\0';
	prs_buff(&numbuf[i]);
}

void
flushb()
{
	if (index)
	{
		buffer[index] = '\0';
		write(1, buffer, length(buffer) - 1);
		index = 0;
	}
}

void
prc_buff(c)
	char c;
{
	if (c)
	{
		if (index + 1 >= BUFLEN)
			flushb();

		buffer[index++] = c;
	}
	else
	{
		flushb();
		write(1, &c, 1);
	}
}

void
prs_buff(s)
	char *s;
{
	register int len = length(s) - 1;

	if (index + len >= BUFLEN)
		flushb();

	if (len >= BUFLEN)
		write(1, s, len);
	else
	{
		movstr(s, &buffer[index]);
		index += len;
	}
}


clear_buff()
{
	index = 0;
}


void
prs_cntl(s)
	char *s;
{
	register char *ptr = buffer;
	int c;

	while (*s != '\0')
	{
		c = *s;
		c &= 0377;

		/* translate a control character into a printable sequence */

		if (c < ' ')
		{       /* assumes ASCII char */
			*ptr++ = '^';
			*ptr++ = (c + '@');    /* assumes ASCII char */
		}
		else if (c == 0177 || (c>=0200 && c<0300 ))
		{       /* '\0177' does not work */
			*ptr++ = '^';
			*ptr++ = '?';
		}
		else
		{       /* printable character */
			*ptr++ = c;
		}

		++s;
	}

	*ptr = '\0';
	prs(buffer);
}


void
prn_buff(n)
	int     n;
{
	itos(n);

	prs_buff(numbuf);
}
