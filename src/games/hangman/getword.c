#include <string.h>
#include <stdlib.h>
#include "hangman.h"

/*
 * abs:
 *	Return the absolute value of an integer
 */
off_t
offabs(i)
off_t	i;
{
	if (i < 0)
		return -(off_t) i;
	else
		return (off_t) i;
}

/*
 * getword:
 *	Get a valid word out of the dictionary file
 */
void
getword()
{
	register FILE		*inf;
	register char		*wp, *gp;

	inf = Dict;
	for (;;) {
		fseek(inf, offabs(random() % Dict_size), 0);
		if (fgets(Word, BUFSIZ, inf) == NULL)
			continue;
		if (fgets(Word, BUFSIZ, inf) == NULL)
			continue;
		Word[strlen(Word) - 1] = '\0';
		if (strlen(Word) < MINLEN)
			continue;
		for (wp = Word; *wp; wp++)
			if (!islower(*wp))
				goto cont;
		break;
cont:		;
	}
	gp = Known;
	wp = Word;
	while (*wp) {
		*gp++ = '-';
		wp++;
	}
	*gp = '\0';
}
