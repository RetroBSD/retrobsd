/*
 *	This routine rolls ndie nside-sided dice.
 */

int
roll(ndie, nsides)
int	ndie, nsides; {

	int	tot;

	tot = 0;
	while (ndie--)
		tot += rand() % nsides + 1;
	return tot;
}
