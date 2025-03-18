/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include	<string.h>
#include	"deck.h"
#include	"cribbage.h"
#include	"cribcur.h"

#define		NTV		10		/* number scores to test */

/* score to test reachability of, and order to test them in */
int		tv[ NTV ]	= { 8, 7, 9, 6, 11, 12, 13, 14, 10, 5 };

/*
 * anysumto returns the index (0 <= i < n) of the card in hand that brings
 * the s up to t, or -1 if there is none
 */
int anysumto(
    CARD		hand[],
    int			n,
    int			s,  
	int			t)
{
	register  int		i;

	for( i = 0; i < n; i++ )  {
	    if(  s + VAL( hand[i].rank )  ==  t  )  return( i );
	}
	return( -1 );
}

/*
 * return the number of cards in h having the given rank value
 */
int numofval(
    CARD		h[],
    int			n,
    int			v)
{
	register  int		i, j;

	j = 0;
	for( i = 0; i < n; i++ )  {
	    if(  VAL( h[i].rank )  ==  v  )  ++j;
	}
	return( j );
}

/*
 * computer chooses what to play in pegging...
 * only called if no playable card will score points
 */
int cchose(
    CARD	h[],
    int		n,
    int		s)
{
	register  int		i, j, l;

	if(  n <= 1  )  return( 0 );
	if(  s < 4  )  {		/* try for good value */
	    if(  ( j = anysumto(h, n, s, 4) )  >=  0  )  return( j );
	    if(  ( j = anysumto(h, n, s, 3) ) >= 0  &&  s == 0  )
								return( j );
	}
	if(  s > 0  &&  s < 20  )  {
	    for( i = 1; i <= 10; i++ )  {	/* try for retaliation to 31 */
		if(  ( j = anysumto(h, n, s, 21-i) )  >=  0  )  {
		    if(  ( l = numofval(h, n, i) )  >  0  )  {
			if(  l > 1  ||  VAL( h[j].rank ) != i  )  return( j );
		    }
		}
	    }
	}
	if(  s < 15  )  {
	    for( i = 0; i < NTV; i++ )  {	/* for retaliation after 15 */
		if(  ( j = anysumto(h, n, s, tv[i]) )  >=  0  )  {
		    if(  ( l = numofval(h, n, 15-tv[i]) )  >  0  )  {
			if(  l > 1  ||  VAL( h[j].rank ) != 15-tv[i]  )  return( j );
		    }
		}
	    }
	}
	j = -1;
	for( i = n - 1; i >= 0; --i )  {	/* remember: h is sorted */
	    l = s + VAL( h[i].rank );
	    if(  l > 31  )  continue;
	    if(  l != 5  &&  l != 10  &&  l != 21  )  {
		j = i;
		break;
	    }
	}
	if(  j >= 0  )  return( j );
	for( i = n - 1; i >= 0; --i )  {
	    l = s + VAL( h[i].rank );
	    if(  l > 31  )  continue;
	    if(  j < 0  )  j = i;
	    if(  l != 5  &&  l != 21  )  {
		j = i;
		break;
	    }
	}
	return( j );
}

/*
 * plyrhand:
 *	Evaluate and score a player hand or crib
 */
BOOLEAN plyrhand(
    CARD	hand[],
    char	*s)
{
    register int	i, j;
    register BOOLEAN	win;
    static char		prompt[BUFSIZ];

    prhand(hand, CINHAND, Playwin, FALSE);
    sprintf(prompt, "Your %s scores ", s);
    i = scorehand(hand, turnover, CINHAND, strcmp(s, "crib") == 0, explain);
    if ((j = number(0, 29, prompt)) == 19)
	j = 0;
    if (i != j) {
	if (i < j) {
	    win = chkscr(&pscore, i);
	    msg("It's really only %d points; I get %d", i, 2);
	    if (!win)
		win = chkscr(&cscore, 2);
	}
	else {
	    win = chkscr(&pscore, j);
	    msg("You should have taken %d, not %d!", i, j);
	}
	if (explain)
	    msg("Explanation: %s", explstr);
	do_wait();
    }
    else
	win = chkscr(&pscore, i);
    return win;
}

/*
 * comphand:
 *	Handle scoring and displaying the computers hand
 */
BOOLEAN comphand(
    CARD	h[],
    char	*s)
{
	register int		j;

	j = scorehand(h, turnover, CINHAND, strcmp(s, "crib") == 0, FALSE);
	prhand(h, CINHAND, Compwin, FALSE);
	msg("My %s scores %d", s, (j == 0 ? 19 : j));
	return chkscr(&cscore, j);
}

int	Lastscore[2] = {-1, -1};

/*
 * prpeg:
 *	Put out the peg character on the score board and put the
 *	score up on the board.
 */
void prpeg(
    register int    score,
    char            peg,
    BOOLEAN         myturn)
{
	register int	y, x;

	if (!myturn)
		y = SCORE_Y + 2;
	else
		y = SCORE_Y + 5;

	if (score <= 0 || score >= glimit) {
		if (peg == '.')
			peg = ' ';
		if (score == 0)
			x = SCORE_X + 2;
		else {
			x = SCORE_X + 2;
			y++;
		}
	}
	else {
		x = (score - 1) % 30;
		if (score > 90 || (score > 30 && score <= 60)) {
			y++;
			x = 29 - x;
		}
		x += x / 5;
		x += SCORE_X + 3;
	}
	mvaddch(y, x, peg);
	mvprintw(SCORE_Y + (myturn ? 7 : 1), SCORE_X + 10, "%3d", score);
}

/*
 * chkscr:
 *	Add inc to scr and test for > glimit, printing on the scoring
 *	board while we're at it.
 */
BOOLEAN chkscr(
    int *scr, 
	int inc)
{
	BOOLEAN		myturn;

	myturn = (scr == &cscore);
	if (inc != 0) {
		prpeg(Lastscore[myturn], '.', myturn);
		Lastscore[myturn] = *scr;
		*scr += inc;
		prpeg(*scr, PEG, myturn);
		refresh();
	}
	return (*scr >= glimit);
}

/*
 * cdiscard -- the computer figures out what is the best discard for
 * the crib and puts the best two cards at the end
 */
void cdiscard(
    BOOLEAN		mycrib)
{
	CARD			d[ CARDS ],  h[ FULLHAND ],  cb[ 2 ];
	register  int		i, j, k;
	int			nc, ns;
	long			sums[ 15 ];
	static  int		undo1[15]   = {0,0,0,0,0,1,1,1,1,2,2,2,3,3,4};
	static  int		undo2[15]   = {1,2,3,4,5,2,3,4,5,3,4,5,4,5,5};

	makedeck( d );
	nc = CARDS;
	for( i = 0; i < knownum; i++ )  {	/* get all other cards */
	    cremove( known[i], d, nc-- );
	}
	for( i = 0; i < 15; i++ )  sums[i] = 0L;
	ns = 0;
	for( i = 0; i < (FULLHAND - 1); i++ )  {
	    cb[0] = chand[i];
	    for( j = i + 1; j < FULLHAND; j++ )  {
		cb[1] = chand[j];
		for( k = 0; k < FULLHAND; k++ )  h[k] = chand[k];
		cremove( chand[i], h, FULLHAND );
		cremove( chand[j], h, FULLHAND - 1 );
		for( k = 0; k < nc; k++ )  {
		    sums[ns] += scorehand( h, d[k], CINHAND, TRUE, FALSE );
		    if( mycrib )  sums[ns] += adjust( cb, d[k] );
		    else	  sums[ns] -= adjust( cb, d[k] );
		}
		++ns;
	    }
	}
	j = 0;
	for( i = 1; i < 15; i++ )  if(  sums[i] > sums[j]  )  j = i;
	for( k = 0; k < FULLHAND; k++ )  h[k] = chand[k];
	cremove( h[ undo1[j] ], chand, FULLHAND );
	cremove( h[ undo2[j] ], chand, FULLHAND - 1 );
	chand[4] = h[ undo1[j] ];
	chand[5] = h[ undo2[j] ];
}

/*
 * returns true if some card in hand can be played without exceeding 31
 */
BOOLEAN anymove(
    CARD		hand[],
    int			n,
    int			sum)
{
	register  int		i, j;

	if(  n < 1  )  return( FALSE );
	j = hand[0].rank;
	for( i = 1; i < n; i++ )  {
	    if(  hand[i].rank < j  )  j = hand[i].rank;
	}
	return(  sum + VAL( j )  <=  31  );
}

/*
 * makeknown remembers all n cards in h for future recall
 */
void makeknown(
    CARD		h[],
    int			n)
{
	register  int		i;

	for( i = 0; i < n; i++ )  {
	    known[ knownum++ ] = h[i];
	}
}
