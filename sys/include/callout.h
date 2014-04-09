/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * The callout structure is for
 * a routine arranging
 * to be called by the clock interrupt
 * (clock.c) with a specified argument,
 * in a specified amount of time.
 * Used, for example, to time tab
 * delays on typewriters.
 *
 * The c_time field is stored in terms of ticks.  Therefore, no callout
 * may be scheduled past around 8 minutes on a 60 HZ machine.  This is
 * good as it avoids long operations on clock ticks.  If you are ever
 * forced to use a long, you might as well start doing the real-time
 * timer as a timeout like 4.3BSD.
 */
struct	callout {
	int	c_time;				/* incremental time */
	caddr_t	c_arg;				/* argument to routine */
	void	(*c_func) (caddr_t);		/* routine */
	struct	callout *c_next;
};

#ifdef KERNEL
extern struct	callout *callfree, callout[], calltodo;
#endif
