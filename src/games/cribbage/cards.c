/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include	<stdio.h>
#include	<stdlib.h>
#include	"deck.h"

/*
 * initialize a deck of cards to contain one of each type
 */
void makedeck(
    CARD	d[])
{
	register  int		i, j, k;
	long			time(long *);

	i = time( (long *) 0 );
	i = ( (i&0xff) << 8 ) | ( (i >> 8)&0xff ) | 1;
	srand( i );
	k = 0;
	for( i = 0; i < RANKS; i++ )  {
	    for( j = 0; j < SUITS; j++ )  {
		d[k].suit = j;
		d[k++].rank = i;
	    }
	}
}

/*
 * given a deck of cards, shuffle it -- i.e. randomize it
 * see Knuth, vol. 2, page 125
 */
void shuffle(
    CARD	d[])
{
	register  int		j, k;
	CARD			c;

	for( j = CARDS; j > 0; --j )  {
	    k = ( rand() >> 4 ) % j;		/* random 0 <= k < j */
	    c = d[j - 1];			/* exchange (j - 1) and k */
	    d[j - 1] = d[k];
	    d[k] = c;
	}
}

/*
 * return true if the two cards are equal...
 */
int eq(
    CARD		a, 
	CARD		b)
{
	return(  ( a.rank == b.rank )  &&  ( a.suit == b.suit )  );
}

/*
 * isone returns TRUE if a is in the set of cards b
 */
BOOLEAN isone(
    CARD		a,
	CARD		b[],
    int			n)
{
	register  int		i;

	for( i = 0; i < n; i++ )  {
	    if(  eq( a, b[i] )   )  return( TRUE );
	}
	return( FALSE );
}

/*
 * remove the card a from the deck d of n cards
 */
void cremove(
    CARD	a, 
	CARD	d[],
    int		n)
{
	register  int		i, j;

	j = 0;
	for( i = 0; i < n; i++ )  {
	    if(  !eq( a, d[i] )  )  d[j++] = d[i];
	}
	if(  j < n  )  d[j].suit = d[j].rank = EMPTY;
}

/*
 * sorthand:
 *	Sort a hand of n cards
 */
void sorthand(
    register CARD	h[],
    int			n)
{
	register CARD		*cp, *endp;
	CARD			c;

	for (endp = &h[n]; h < endp - 1; h++)
	    for (cp = h + 1; cp < endp; cp++)
		if ((cp->rank < h->rank) ||
		     (cp->rank == h->rank && cp->suit < h->suit)) {
		    c = *h;
		    *h = *cp;
		    *cp = c;
		}
}
