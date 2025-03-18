/*
 *	This routine rolls ndie nside-sided dice.
 */
#include <stdlib.h>

int
roll(
int	ndie, 
int nsides) {

	int	tot;

	tot = 0;
	while (ndie--)
		tot += random() % nsides + 1;
	return tot;
}
