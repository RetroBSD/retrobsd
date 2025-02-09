/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sgtty.h>
#include <ctype.h>

/*
 * Put the character string cp out, with padding.
 * The number of affected lines is affcnt, and the routine
 * used to output one character is outc.
 */
void
tputs(char *cp, int affcnt, int (*outc)(int))
{
	register int i = 0;

	if (cp == 0)
		return;

	/*
	 * Convert the number representing the delay.
	 */
	if (isdigit(*cp)) {
		do
			i = i * 10 + *cp++ - '0';
		while (isdigit(*cp));
	}
	i *= 10;
	if (*cp == '.') {
		cp++;
		if (isdigit(*cp))
			i += *cp - '0';
		/*
		 * Only one digit to the right of the decimal point.
		 */
		while (isdigit(*cp))
			cp++;
	}

	/*
	 * If the delay is followed by a `*', then
	 * multiply by the affected lines count.
	 */
	if (*cp == '*')
		cp++, i *= affcnt;

	/*
	 * The guts of the string.
	 */
	while (*cp)
		(*outc)(*cp++);
}
