/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 *	This routine fills in "def" with the full name of the terminal.
 * This is assumed to be the last name in the list of aliases.
 *
 */
char *
fullname(char *bp, char *def)
{
	char	*cp;

	*def = 0;			/* in case no name */

	while (*bp && *bp != ':') {
		cp = def;		/* start of answer */
		while (*bp && *bp != ':' && *bp != '|') {
			*cp++ = *bp++;	/* copy name over */
		}
		*cp = 0;		/* zero end of name */
		if (*bp == '|') {
			bp++;		/* skip over '|' if that is case */
		}
	}
	return(def);
}
