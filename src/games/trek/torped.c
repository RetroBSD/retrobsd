/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
# include	<stdio.h>
# include	<stdlib.h>
# include	"trek.h"
# include	"getpar.h"

/*
**  RANDOMIZE COURSE
**
**	This routine randomizes the course for torpedo number 'n'.
**	Other things handled by this routine are misfires, damages
**	to the tubes, etc.
*/
static int
randcourse(
        int	n)
{
	double			r;
	register int		d;

	d = ((franf() + franf()) - 1.0) * 20;
	if (abs(d) > 12)
	{
		printf("Photon tubes misfire");
		if (n < 0)
			printf("\n");
		else
			printf(" on torpedo %d\n", n);
		if (ranf(2))
		{
			damage(TORPED, 0.2 * abs(d) * (franf() + 1.0));
		}
		d *= 1.0 + 2.0 * franf();
	}
	if (Ship.shldup || Ship.cond == DOCKED)
	{
		r = Ship.shield;
		r = 1.0 + r / Param.shield;
		if (Ship.cond == DOCKED)
			r = 2.0;
		d *= r;
	}
	return (d);
}

/*
**  PHOTON TORPEDO CONTROL
**
**	Either one or three photon torpedoes are fired.  If three
**	are fired, it is called a "burst" and you also specify
**	a spread angle.
**
**	Torpedoes are never 100% accurate.  There is always a random
**	cludge factor in their course which is increased if you have
**	your shields up.  Hence, you will find that they are more
**	accurate at close range.  However, they have the advantage that
**	at long range they don't lose any of their power as phasers
**	do, i.e., a hit is a hit is a hit, by any other name.
**
**	When the course spreads too much, you get a misfire, and the
**	course is randomized even more.  You also have the chance that
**	the misfire damages your torpedo tubes.
*/
void
torped()
{
	register int		ix, iy;
	double			x, y, dx, dy;
	double			angle;
	int			course, course2;
	register int		k;
	double			bigger;
	double			sectsize;
	int			burst;
	int			n;

	if (Ship.cloaked) {
	        printf("Federation regulations do not permit attack while cloaked.\n");
		return;
	}
	if (check_out(TORPED))
		return;
	if (Ship.torped <= 0) {
	        printf("All photon torpedos expended\n");
		return;
	}

	/* get the course */
	course = getintpar("Torpedo course");
	if (course < 0 || course > 360)
		return;
	burst = -1;

	/* need at least three torpedoes for a burst */
	if (Ship.torped < 3)
	{
		printf("No-burst mode selected\n");
		burst = 0;
	}
	else
	{
		/* see if the user wants one */
		if (!testnl())
		{
			k = ungetc(fgetc(stdin), stdin);
			if (k >= '0' && k <= '9')
				burst = 1;
		}
	}
	if (burst < 0)
	{
		burst = getynpar("Do you want a burst");
	}
	if (burst)
	{
		burst = getintpar("burst angle");
		if (burst <= 0)
			return;
		if (burst > 15) {
		        printf("Maximum burst angle is 15 degrees\n");
			return;
                }
	}
	sectsize = NSECTS;
	n = -1;
	if (burst)
	{
		n = 1;
		course -= burst;
	}
	for (; n && n <= 3; n++)
	{
		/* select a nice random course */
		course2 = course + randcourse(n);
		angle = course2 * 0.0174532925;			/* convert to radians */
		dx = -cos(angle);
		dy =  sin(angle);
		bigger = fabs(dx);
		x = fabs(dy);
		if (x > bigger)
			bigger = x;
		dx /= bigger;
		dy /= bigger;
		x = Ship.sectx + 0.5;
		y = Ship.secty + 0.5;
		if (Ship.cond != DOCKED)
			Ship.torped -= 1;
		printf("Torpedo track");
		if (n > 0)
			printf(", torpedo number %d", n);
		printf(":\n%6.1f\t%4.1f\n", x, y);
		while (1)
		{
			ix = x += dx;
			iy = y += dy;
			if (x < 0.0 || x >= sectsize || y < 0.0 || y >= sectsize)
			{
				printf("Torpedo missed\n");
				break;
			}
			printf("%6.1f\t%4.1f\n", x, y);
			switch (Sect[ix][iy])
			{
			  case EMPTY:
				continue;

			  case HOLE:
				printf("Torpedo disappears into a black hole\n");
				break;

			  case KLINGON:
				for (k = 0; k < Etc.nkling; k++)
				{
					if (Etc.klingon[k].x != ix || Etc.klingon[k].y != iy)
						continue;
					Etc.klingon[k].power -= 500 + ranf(501);
					if (Etc.klingon[k].power > 0)
					{
						printf("*** Hit on Klingon at %d,%d: extensive damages\n",
							ix, iy);
						break;
					}
					killk(ix, iy);
					break;
				}
				break;

			  case STAR:
				nova(ix, iy);
				break;

			  case INHABIT:
				kills(ix, iy, -1);
				break;

			  case BASE:
				killb(Ship.quadx, Ship.quady);
				Game.killb += 1;
				break;
			  default:
				printf("Unknown object %c at %d,%d destroyed\n",
					Sect[ix][iy], ix, iy);
				Sect[ix][iy] = EMPTY;
				break;
			}
			break;
		}
		if (damaged(TORPED) || Quad[Ship.quadx][Ship.quady].stars < 0)
			break;
		course += burst;
	}
	Move.free = 0;
}
