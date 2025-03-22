/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
# include	"trek.h"

/*
**  sort klingons
**
**	bubble sort on ascending distance
*/
void
sortkl()
{
	struct kling		t;
	register int		f, i, m;

	m = Etc.nkling - 1;
	f = 1;
	while (f)
	{
		f = 0;
		for (i = 0; i < m; i++)
			if (Etc.klingon[i].dist > Etc.klingon[i+1].dist)
			{
				bmove(&Etc.klingon[i], &t, sizeof t);
				bmove(&Etc.klingon[i+1], &Etc.klingon[i], sizeof t);
				bmove(&t, &Etc.klingon[i+1], sizeof t);
				f = 1;
			}
	}
}

/*
**  compute klingon distances
**
**	The klingon list has the distances for all klingons recomputed
**	and sorted.  The parameter is a Boolean flag which is set if
**	we have just entered a new quadrant.
**
**	This routine is used every time the Enterprise or the Klingons
**	move.
*/
void
compkldist(
        int	f)		/* set if new quadrant */
{
	register int		i, dx, dy;
	double			d;
	double			temp;

	if (Etc.nkling == 0)
		return;
	for (i = 0; i < Etc.nkling; i++)
	{
		/* compute distance to the Klingon */
		dx = Ship.sectx - Etc.klingon[i].x;
		dy = Ship.secty - Etc.klingon[i].y;
		d = dx * dx + dy * dy;
		d = sqrt(d);

		/* compute average of new and old distances to Klingon */
		if (!f)
		{
			temp = Etc.klingon[i].dist;
			Etc.klingon[i].avgdist = 0.5 * (temp + d);
		}
		else
		{
			/* new quadrant: average is current */
			Etc.klingon[i].avgdist = d;
		}
		Etc.klingon[i].dist = d;
	}

	/* leave them sorted */
	sortkl();
}
