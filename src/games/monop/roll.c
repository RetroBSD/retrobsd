/*
 *	This routine rolls ndie nside-sided dice.
 */
#include <stdlib.h>

int
roll(ndie, nsides)
int	ndie, nsides; {

	int	tot;

	tot = 0;
	while (ndie--)
		tot += random() % nsides + 1;
	return tot;
}
