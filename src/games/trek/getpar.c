/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
# include	<stdio.h>
# include	<stdlib.h>
# include	"getpar.h"

/**
 **	test for valid terminator
 **/
static int
testterm()
{
	register char		c;

	c = fgetc(stdin);
	if (c == '.')
		return (0);
	if (c == '\n' || c == ';')
		ungetc(c, stdin);
	return (1);
}

/**
 **	get integer parameter
 **/
int
getintpar(s)
        char	*s;
{
	register int	i;
	int		n;

	while (1)
	{
		if (testnl() && s)
			printf("%s: ", s);
		i = scanf("%d", &n);
		if (i < 0)
			exit(1);
		if (i > 0 && testterm())
			return (n);
		printf("invalid input; please enter an integer\n");
		skiptonl(0);
	}
}

/**
 **	get floating parameter
 **/
double
getfltpar(s)
        char	*s;
{
	register int		i;
	double			d;

	while (1)
	{
		if (testnl() && s)
			printf("%s: ", s);
		i = scanf("%lf", &d);
		if (i < 0)
			exit(1);
		if (i > 0 && testterm())
			return (d);
		printf("invalid input; please enter a double\n");
		skiptonl(0);
	}
}

/**
 **	get yes/no parameter
 **/
struct cvntab	Yntab[] = {
	{ "y",	"es",	(void (*)())1,	0 },
	{ "n",	"o",	(void (*)())0,	0 },
	{ 0 },
};

int
getynpar(s)
        char	*s;
{
	struct cvntab		*r;

	r = getcodpar(s, Yntab);
	return ((int) r->value);
}


/*
**  STRING CONCATENATE
**
**	The strings `s1' and `s2' are concatenated and stored into
**	`s3'.  It is ok for `s1' to equal `s3', but terrible things
**	will happen if `s2' equals `s3'.  The return value is is a
**	pointer to the end of `s3' field.
*/
char *
concat(s1, s2, s3)
        char	*s1, *s2, *s3;
{
	register char		*p;
	register char		*q;

	p = s3;
	q = s1;
	while (*q)
		*p++ = *q++;
	q = s2;
	while (*q)
		*p++ = *q++;
	*p = 0;
	return (p);
}


/**
 **	get coded parameter
 **/
struct cvntab *
getcodpar(s, tab)
        char		*s;
        struct cvntab	tab[];
{
	char				input[100];
	register struct cvntab		*r;
	int				flag;
	register char			*p, *q;
	int				c;
	int				f;

	flag = 0;
	while (1)
	{
		flag |= (f = testnl());
		if (flag)
			printf("%s: ", s);
		if (f)
			fgetc(stdin);		/* throw out the newline */
		scanf("%*[ \t;]");
		if ((c = scanf("%[^ \t;\n]", input)) < 0)
			exit(1);
		if (c == 0)
			continue;
		flag = 1;

		/* if command list, print four per line */
		if (input[0] == '?' && input[1] == 0)
		{
			c = 4;
			for (r = tab; r->abrev; r++)
			{
				concat(r->abrev, r->full, input);
				printf("%14.14s", input);
				if (--c > 0)
					continue;
				c = 4;
				printf("\n");
			}
			if (c != 4)
				printf("\n");
			continue;
		}

		/* search for in table */
		for (r = tab; r->abrev; r++)
		{
			p = input;
			for (q = r->abrev; *q; q++)
				if (*p++ != *q)
					break;
			if (!*q)
			{
				for (q = r->full; *p && *q; q++, p++)
					if (*p != *q)
						break;
				if (!*p || !*q)
					break;
			}
		}

		/* check for not found */
		if (!r->abrev)
		{
			printf("invalid input; ? for valid inputs\n");
			skiptonl(0);
		}
		else
			return (r);
	}
}


/**
 **	get string parameter
 **/
void
getstrpar(s, r, l, t)
        char	*s;
        char	*r;
        int	l;
        char	*t;
{
	register int	i;
	char		format[20];
	register int	f;

	if (t == 0)
		t = " \t\n;";
	sprintf(format, "%%%d[^%s]", l, t);
	while (1)
	{
		if ((f = testnl()) && s)
			printf("%s: ", s);
		if (f)
			fgetc(stdin);
		scanf("%*[\t ;]");
		i = scanf(format, r);
		if (i < 0)
			exit(1);
		if (i != 0)
			return;
	}
}


/**
 **	test if newline is next valid character
 **/
int
testnl()
{
	register char		c;

	while ((c = fgetc(stdin)) != '\n')
		if ((c >= '0' && c <= '9') || c == '.' || c == '!' ||
				(c >= 'A' && c <= 'Z') ||
				(c >= 'a' && c <= 'z') || c == '-')
		{
			ungetc(c, stdin);
			return(0);
		}
	ungetc(c, stdin);
	return (1);
}


/**
 **	scan for newline
 **/
void
skiptonl(c)
        int	c;
{
	while (c != '\n')
		if (!(c = fgetc(stdin)))
			return;
	ungetc('\n', stdin);
}


/*
**  TEST FOR SPECIFIED DELIMETER
**
**	The standard input is scanned for the parameter.  If found,
**	it is thrown away and non-zero is returned.  If not found,
**	zero is returned.
*/
int
readdelim(d)
        int	d;
{
	register char	c;

	while ((c = fgetc(stdin))) {
		if (c == d)
			return (1);
		if (c == ' ')
			continue;
		ungetc(c, stdin);
		break;
	}
	return (0);
}
