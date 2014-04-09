/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <unistd.h>

extern char _end[];
const char *_curbrk = _end;

void *
sbrk (incr)
	int incr;
{
	void *oldbrk = (void*) _curbrk;

	if (incr != 0) {
		/* calculate and pass break address */
		const void *addr = _curbrk + incr;
		if (_brk (addr) != -1) {
			/* add increment to curbrk */
			_curbrk = addr;
		}
	}
	/* return old break address */
	return oldbrk;
}

void *
brk (addr)
	const void *addr;
{
	int ret;

	if (addr < (void*) _end)	/* break request too low? */
		addr = _end;		/* yes, knock the request up to _end */
	ret = _brk (addr);		/* ask for break */
	if (ret != -1)
		_curbrk = addr;		/* and remember it if it succeeded */
	return (void*) ret;
}
