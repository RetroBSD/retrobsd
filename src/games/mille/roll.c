#include "mille.h"
#include <stdlib.h>

/*
 *	This routine rolls ndie nside-sided dice.
 *
 * @(#)roll.c	1.1 (Berkeley) 4/1/82
 *
 */

int
roll(ndie, nsides)
int	ndie, nsides; {

	int	tot;

	tot = 0;
	while (ndie--)
		tot += random() % nsides + 1;
	return tot;
}
