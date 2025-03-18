/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "back.h"

#ifdef DEBUG
#ifdef CROSS
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif
FILE	*trace;
#endif

/*
 * dble()
 *	Have the current player double and ask opponent to accept.
 */
void
dble ()
{
	register int	resp;			/* response to y/n */

	for (;;)  {
		writel (" doubles.");		/* indicate double */

		if (cturn == -pnum)  {		/* see if computer accepts */
			if (dblgood())  {	    /* guess not */
				writel ("  Declined.\n");
				nexturn();
				cturn *= -2;	    /* indicate loss */
				return;
			} else  {		    /* computer accepts */
				writel ("  Accepted.\n");
				gvalue *= 2;	    /* double game value */
				dlast = cturn;
				if (tflag)
					gwrite();
				return;
			}
		}

						/* ask if player accepts */
		writel ("  Does ");
		writel (cturn == 1? color[2]: color[3]);
		writel (" accept?");

						/* get response from yorn,
						 * a "2" means he said "p"
						 * for print board. */
		if ((resp = yorn ('R')) == 2)  {
			writel ("  Reprint.\n");
			buflush();
			wrboard();
			writel (*Colorptr);
			continue;
		}

						/* check response */
		if (resp)  {
						    /* accepted */
			gvalue *= 2;
			dlast = cturn;
			if (tflag)
				gwrite();
			return;
		}

		nexturn ();			/* declined */
		cturn *= -2;
		return;
	}
}

static int
eval ()
{
	register int	i, j;

	for (j = i = 0; i < 26; i++)
		j += (board[i] >= 0 ? i*board[i] : (25-i)*board[i]);

	if (off[1] >= 0)
		j += 25*off[1];
	else
		j += 25*(off[1]+15);

	if (off[0] >= 0)
		j -= 25*off[0];
	else
		j -= 25*(off[0]+15);
	return (j);
}

/*
 * dblgood ()
 *	Returns 1 if the computer would double in this position.  This
 * is not an exact science.  The computer will decline a double that he
 * would have made.  Accumulated judgments are kept in the variable n,
 * which is in "pips", i.e., the position of each man summed over all
 * men, with opponent's totals negative.  Thus, n should have a positive
 * value of 7 for each move ahead, or a negative value of 7 for each one
 * behind.
 */
int
dblgood ()
{
	register int	n;			/* accumulated judgment */
	register int	OFFC = *offptr;		/* no. of computer's men off */
	register int	OFFO = *offopp;		/* no. of player's men off */

#ifdef DEBUG
	register int	i;
	if (trace == NULL)
		trace = fopen ("bgtrace","w");
#endif

						/* get real pip value */
	n = eval()*cturn;
#ifdef DEBUG
	fputs ("\nDoubles:\nBoard: ",trace);
	for (i = 0; i < 26; i++)
		fprintf (trace," %d",board[i]);
	fprintf (trace,"\n\tpip = %d, ",n);
#endif

						/* below adjusts pip value
						 * according to position
						 * judgments */

						/* check men moving off
						 * board */
	if (OFFC > -15 || OFFO > -15)  {
		if (OFFC < 0 && OFFO < 0)  {
			OFFC += 15;
			OFFO += 15;
			n +=((OFFC-OFFO)*7)/2;
		} else if (OFFC < 0)  {
			OFFC += 15;
			n -= OFFO*7/2;
		} else if (OFFO < 0)  {
			OFFO += 15;
			n += OFFC*7/2;
		}
		if (OFFC < 8 && OFFO > 8)
			n -= 7;
		if (OFFC < 10 && OFFO > 10)
			n -= 7;
		if (OFFC < 12 && OFFO > 12)
			n -= 7;
		if (OFFO < 8 && OFFC > 8)
			n += 7;
		if (OFFO < 10 && OFFC > 10)
			n += 7;
		if (OFFO < 12 && OFFC > 12)
			n += 7;
		n += ((OFFC-OFFO)*7)/2;
	}

#ifdef DEBUG
	fprintf (trace,"off = %d, ",n);
#endif

						/* see if men are trapped */
	n -= freemen(bar);
	n += freemen(home);
	n += trapped(home,-cturn);
	n -= trapped(bar,cturn);

#ifdef DEBUG
	fprintf (trace,"free = %d\n",n);
	fprintf (trace,"\tOFFC = %d, OFFO = %d\n",OFFC,OFFO);
	fflush (trace);
#endif

						/* double if 2-3 moves ahead */
	if (n > 10+rnum(7))
		return(1);
	return (0);
}

int
freemen (
        int	b)
{
	register int	i, inc, lim;

	odds(0,0,0);
	if (board[b] == 0)
		return (0);
	inc = (b == 0? 1: -1);
	lim = (b == 0? 7: 18);
	for (i = b+inc; i != lim; i += inc)
		if (board[i]*inc < -1)
			odds(abs(b-i),0,abs(board[b]));
	if (abs(board[b]) == 1)
		return ((36-count())/5);
	return (count()/5);
}

int
trapped (
        int	n, int inc)
{
	register int	i, j, k;
	int		c, l, ct;

	ct = 0;
	l = n+7*inc;
	for (i = n+inc; i != l; i += inc)  {
		odds (0,0,0);
		c = abs(i-l);
		if (board[i]*inc > 0)  {
			for (j = c; j < 13; j++)
				if (board[i+inc*j]*inc < -1)  {
					if (j < 7)
						odds (j,0,1);
					for (k = 1; k < 7 && k < j; k++)
						if (j-k < 7)
							odds (k,j-k,1);
				}
			ct += abs(board[i])*(36-count());
		}
	}
	return (ct/5);
}
