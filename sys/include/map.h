/*
 * Resource Allocation Maps.
 *
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Associated routines manage allocation of an address space using
 * an array of segment descriptors.
 *
 * Malloc and mfree allocate and free the resource described
 * by the resource map.  If the resource map becomes too fragmented
 * to be described in the available space, then some of the resource
 * is discarded.  This may lead to critical shortages,
 * but is better than not checking (as the previous versions of
 * these routines did) or giving up and calling panic().
 *
 * N.B.: The address 0 in the resource address space is not available
 * as it is used internally by the resource map routines.
 */
struct map {
	struct mapent	*m_map;		/* start of the map */
	struct mapent	*m_limit;	/* address of last slot in map */
	char	*m_name;		/* name of resource */
/* we use m_name when the map overflows, in warning messages */
};

struct mapent {
	size_t	m_size;		/* size of this segment of the map */
	size_t	m_addr;		/* resource-space addr of start of segment */
};

#ifdef KERNEL
extern struct map swapmap[];	/* space for swap allocation */

/*
 * Allocate units from the given map.
 */
size_t malloc (struct map *mp, size_t nbytes);

/*
 * Free the previously allocated units at addr into the specified map.
 */
void mfree (struct map *mp, size_t nbytes, size_t addr);

/*
 * Allocate resources for the three segments of a process.
 */
size_t malloc3 (struct map *mp, size_t d_size, size_t s_size, size_t u_size, size_t a[3]);

#endif
