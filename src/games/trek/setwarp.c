/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
# include	"trek.h"
# include	"getpar.h"

/*
**  SET WARP FACTOR
**
**	The warp factor is set for future move commands.  It is
**	checked for consistancy.
*/
void
setwarp()
{
	double	warpfac;

	warpfac = getfltpar("Warp factor");
	if (warpfac < 0.0)
		return;
	if (warpfac < 1.0) {
	        printf("Minimum warp speed is 1.0\n");
		return;
        }
	if (warpfac > 10.0) {
	        printf("Maximum speed is warp 10.0\n");
		return;
        }
	if (warpfac > 6.0)
		printf("Damage to warp engines may occur above warp 6.0\n");
	Ship.warp = warpfac;
	Ship.warp2 = Ship.warp * warpfac;
	Ship.warp3 = Ship.warp2 * warpfac;
}
