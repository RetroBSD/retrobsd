/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include "param.h"
#include "systm.h"
#include "map.h"
#include "vm.h"

/*
 * Resource map handling routines.
 *
 * A resource map is an array of structures each of which describes a
 * segment of the address space of an available resource.  The segments
 * are described by their base address and length, and sorted in address
 * order.  Each resource map has a fixed maximum number of segments
 * allowed.  Resources are allocated by taking part or all of one of the
 * segments of the map.
 *
 * Returning of resources will require another segment if the returned
 * resources are not adjacent in the address space to an existing segment.
 * If the return of a segment would require a slot which is not available,
 * then one of the resource map segments is discarded after a warning is
 * printed.
 *
 * Returning of resources may also cause the map to collapse by coalescing
 * two existing segments and the returned space into a single segment.  In
 * this case the resource map is made smaller by copying together to fill
 * the resultant gap.
 *
 * N.B.: the current implementation uses a dense array and does not admit
 * the value ``0'' as a legal address or size, since that is used as a
 * delimiter.
 */

/*
 * Allocate 'size' units from the given map.  Return the base of the
 * allocated space.  In a map, the addresses are increasing and the
 * list is terminated by a 0 size.
 *
 * Algorithm is first-fit.
 */
size_t
malloc (mp, size)
	struct map *mp;
	register size_t size;
{
	register struct mapent *bp, *ep;
	size_t addr;

	if (! size)
		panic ("malloc: size = 0");
	/*
	 * Search for a piece of the resource map which has enough
	 * free space to accomodate the request.
	 */
	for (bp = mp->m_map; bp->m_size; ++bp)
		if (bp->m_size >= size) {
			/*
			 * Allocate from the map.  If we allocated the entire
			 * piece, move the rest of the map to the left.
			 */
			addr = bp->m_addr;
			bp->m_size -= size;
			if (bp->m_size)
				bp->m_addr += size;
			else for (ep = bp;; ++ep) {
				*ep = *++bp;
				if (!bp->m_size)
					break;
			}
			return(addr);
		}
	/* no entries big enough */
	return 0;
}

/*
 * Free the previously allocated size units at addr into the specified
 * map.  Sort addr into map and combine on one or both ends if possible.
 */
void
mfree (mp, size, addr)
	struct map *mp;
	size_t size;
	register size_t addr;
{
	register struct mapent *bp, *ep;
	struct mapent *start;

	if (! size)
		return;
	/* the address must not be 0, or the protocol has broken down. */
	if (! addr)
		panic ("mfree: addr = 0");

	/*
	 * locate the piece of the map which starts after the
	 * returned space (or the end of the map).
	 */
	bp = mp->m_map;
	/* printf ("mfree (size=%u, addr=%u) m_map = %08x\n", size, addr, bp); */

	while (bp->m_size && bp->m_addr <= addr) {
		/*printf ("skip m_map[%d]: m_addr %u <= addr %u\n", bp - mp->m_map, bp->m_addr, addr);*/
		++bp;
	}

	/* if there is a piece on the left abutting us, combine with it. */
	ep = bp - 1;
	if (bp != mp->m_map && ep->m_addr + ep->m_size >= addr) {
#ifdef DIAGNOSTIC
		/* any overlap is an internal error */
		if (ep->m_addr + ep->m_size > addr)
			panic("mfree overlap #1");
#endif
		/* add into piece on the left by increasing its size. */
		ep->m_size += size;

		/*
		 * if the combined piece abuts the piece on the right now,
		 * compress it in also, by shifting the remaining pieces
		 * of the map over.
		 */
		if (bp->m_size && addr + size >= bp->m_addr) {
#ifdef DIAGNOSTIC
			if (addr + size > bp->m_addr)
				panic("mfree overlap #2");
#endif
			ep->m_size += bp->m_size;
			do {
				*++ep = *++bp;
			} while (bp->m_size);
		}
		return;
	}

	/* if doesn't abut on the left, check for abutting on the right. */
	if (bp->m_size && addr + size >= bp->m_addr) {
#ifdef DIAGNOSTIC
		if (addr + size > bp->m_addr)
			panic("mfree overlap #3");
#endif
		bp->m_addr = addr;
		bp->m_size += size;
		return;
	}

	/* doesn't abut.  Make a new entry and check for map overflow. */
	for (start = bp; bp->m_size; ++bp);
	if (++bp > mp->m_limit)
		/*
		 * too many segments; if this happens, the correct fix
		 * is to make the map bigger; you can't afford to lose
		 * chunks of the map.  If you need to implement recovery,
		 * use the above "for" loop to find the smallest entry
		 * and toss it.
		 */
		printf("%s: overflow, lost %u clicks at 0%o\n",
		    mp->m_name, size, addr);
	else {
		for (ep = bp - 1; ep >= start; *bp-- = *ep--);
		start->m_addr = addr;
		start->m_size = size;
	}
}

/*
 * Allocate resources for the three segments of a process (data, stack
 * and u.), attempting to minimize the cost of failure part-way through.
 * Since the segments are located successively, it is best for the sizes
 * to be in decreasing order; generally, data, stack, then u. will be
 * best.  Returns NULL on failure, address of u. on success.
 */
size_t
malloc3 (mp, d_size, s_size, u_size, a)
	struct map *mp;
	size_t d_size, s_size, u_size;
	size_t a[3];
{
	register struct mapent *bp, *remap;
	register int next;
	struct mapent *madd[3];
	size_t sizes[3];
	int found;

	sizes[0] = d_size;
	sizes[1] = s_size;
	sizes[2] = u_size;
	/*
	 * note, this has to work for d_size and s_size of zero,
	 * since init() comes in that way.
	 */
	madd[0] = madd[1] = madd[2] = remap = NULL;
	for (found = 0, bp = mp->m_map; bp->m_size; ++bp)
		for (next = 0; next < 3; ++next)
			if (!madd[next] && sizes[next] <= bp->m_size) {
				madd[next] = bp;
				bp->m_size -= sizes[next];
				if (!bp->m_size && !remap)
					remap = bp;
				if (++found == 3)
					goto resolve;
			}

	/* couldn't get it all; restore the old sizes, try again */
	for (next = 0; next < 3; ++next)
		if (madd[next])
			madd[next]->m_size += sizes[next];
	return 0;

resolve:
	/* got it all, update the addresses. */
	for (next = 0; next < 3; ++next) {
		bp = madd[next];
		a[next] = bp->m_addr;
		bp->m_addr += sizes[next];
	}

	/* remove any entries of size 0; addr of 0 terminates */
	if (remap)
		for (bp = remap + 1;; ++bp)
			if (bp->m_size || !bp->m_addr) {
				*remap++ = *bp;
				if (!bp->m_addr)
					break;
			}
	return(a[2]);
}
