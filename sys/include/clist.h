/*
 * Raw structures for the character list routines.
 *
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
struct cblock {
	struct cblock *c_next;
	char	c_info [CBSIZE];
};

#ifdef KERNEL
extern struct cblock cfree[];
struct	cblock *cfreelist;
int	cfreecount;
#endif
