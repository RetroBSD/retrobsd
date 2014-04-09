/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)getpar.h	5.1 (Berkeley) 5/30/85
 */

struct cvntab		/* used for getcodpar() paramater list */
{
	char	*abrev;
	char	*full;
	void	(*value)();
	int	value2;
};

void skiptonl(int c);
void getstrpar(char *s, char *r, int l, char *t);

int getintpar(char *s);
int getynpar(char *s);
int testnl(void);

double getfltpar(char *s);

struct cvntab *getcodpar(char *s, struct cvntab *tab);
