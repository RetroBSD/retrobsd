/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
# include	"trek.h"
# include	<float.h>

/*
**  SCHEDULE AN EVENT
**
**	An event of type 'type' is scheduled for time NOW + 'offset'
**	into the first available slot.  'x', 'y', and 'z' are
**	considered the attributes for this event.
**
**	The address of the slot is returned.
*/
struct event *
schedule(
        int	type,
        double	offset,
        int	x, 
		int y,
        int	z)
{
	register struct event	*e;
	register int		i;
	double			date;

	date = Now.date + offset;
	for (i = 0; i < MAXEVENTS; i++)
	{
		e = &Event[i];
		if (e->evcode)
			continue;
		/* got a slot */
#		ifdef xTRACE
		if (Trace)
			printf("schedule: type %d @ %.2f slot %d parm %d %d %d\n",
				type, date, i, x, y, z);
#		endif
		e->evcode = type;
		e->date = date;
		e->x = x;
		e->y = y;
		e->systemname = z;
		Now.eventptr[type] = e;
		return (e);
	}
	syserr("Cannot schedule event %d parm %d %d %d", type, x, y, z);
	return 0;
}


/*
**  RESCHEDULE AN EVENT
**
**	The event pointed to by 'e' is rescheduled to the current
**	time plus 'offset'.
*/
void
reschedule(
        struct event	*e1,
        double		offset)
{
	double			date;
	register struct event	*e;

	e = e1;

	date = Now.date + offset;
	e->date = date;
#	ifdef xTRACE
	if (Trace)
		printf("reschedule: type %d parm %d %d %d @ %.2f\n",
			e->evcode, e->x, e->y, e->systemname, date);
#	endif
}


/*
**  UNSCHEDULE AN EVENT
**
**	The event at slot 'e' is deleted.
*/
void
unschedule(
        struct event	*e1)
{
	register struct event	*e;

	e = e1;

#	ifdef xTRACE
	if (Trace)
		printf("unschedule: type %d @ %.2f parm %d %d %d\n",
			e->evcode, e->date, e->x, e->y, e->systemname);
#	endif
	Now.eventptr[e->evcode & E_EVENT] = 0;
	e->date = DBL_MAX;
	e->evcode = 0;
}


/*
**  Abreviated schedule routine
**
**	Parameters are the event index and a factor for the time
**	figure.
*/
struct event *
xsched(
        int	ev1,
        int	factor,
        int	x, 
		int y,
		int z)
{
	register int	ev;

	ev = ev1;
	return (schedule(ev, -Param.eventdly[ev] * Param.time * log(franf()) / factor, x, y, z));
}


/*
**  Simplified reschedule routine
**
**	Parameters are the event index, the initial date, and the
**	division factor.  Look at the code to see what really happens.
*/
void
xresched(
        struct event	*e1,
        int		ev1,
        int		factor)
{
	register int		ev;
	register struct event	*e;

	ev = ev1;
	e = e1;
	reschedule(e, -Param.eventdly[ev] * Param.time * log(franf()) / factor);
}
