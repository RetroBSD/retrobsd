/*
 * Copyright (c) 1987 Regents of the University of California.
 * This file may be freely redistributed provided that this
 * notice remains attached.
 */
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <strings.h>
#include <tzfile.h>

/*
 * timezone --
 *	The arguments are the number of minutes of time you are westward
 *	from Greenwich and whether DST is in effect.  It returns a string
 *	giving the name of the local timezone.  Should be replaced, in the
 *	application code, by a call to localtime.
 */

static char	czone[TZ_MAX_CHARS];		/* space for zone name */

char *
timezone(int zone, int dst)
{
	register char	*beg,
			*end;

        beg = getenv("TZNAME");
	if (beg) {                              /* set in environment */
	        end = index(beg, ',');
		if (end) {                      /* "PST,PDT" */
			if (dst)
				return(++end);
			*end = '\0';
			(void)strncpy(czone,beg,sizeof(czone) - 1);
			czone[sizeof(czone) - 1] = '\0';
			*end = ',';
			return(czone);
		}
		return(beg);
	}
	return(tztab(zone,dst));	/* default: table or created zone */
}

static struct zone {
	int	offset;
	char	*stdzone;
	char	*dlzone;
} zonetab[] = {
	{ -1*60,	"MET",	"MET DST"   },	/* Middle European */
	{ -2*60,	"EET",	"EET DST"   },	/* Eastern European */
	{ 4*60,         "AST",	"ADT"       },	/* Atlantic */
	{ 5*60,         "EST",	"EDT"       },	/* Eastern */
	{ 6*60,         "CST",	"CDT"       },	/* Central */
	{ 7*60,         "MST",	"MDT"       },	/* Mountain */
	{ 8*60,         "PST",	"PDT"       },	/* Pacific */
	{ 0,            "GMT",	0           },	/* Greenwich */
	{ -10*60,	"EST",	"EST"       },	/* Aust: Eastern */
        { -10*60+30,	"CST",	"CST"       },	/* Aust: Central */
	{ -8*60,	"WST",	0           },	/* Aust: Western */
	{ -1 },
};

/*
 * tztab --
 *	check static tables or create a new zone name; broken out so that
 *	we can make a guess as to what the zone is if the standard tables
 *	aren't in place in /usr/share/misc.  DO NOT USE THIS ROUTINE OUTSIDE
 *	OF THE STANDARD LIBRARY.
 */
char *
tztab(int zone, int dst)
{
	register struct zone	*zp;
	register char	sign;

	for (zp = zonetab; zp->offset != -1;++zp)	/* static tables */
		if (zp->offset == zone) {
			if (dst && zp->dlzone)
				return(zp->dlzone);
			if (!dst && zp->stdzone)
				return(zp->stdzone);
		}

	if (zone < 0) {					/* create one */
		zone = -zone;
		sign = '+';
	}
	else
		sign = '-';
	(void)sprintf(czone,"GMT%c%d:%02d",sign,zone / 60,zone % 60);
	return(czone);
}
