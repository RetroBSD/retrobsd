/*
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 */
#include <math.h>

/*
 * isnan(x) returns 1 is x is nan, else 0;
 * no branching!
 */
int isnan (double x)
{
	union {
		double value;
		unsigned long long bits;
	} u;
	long hx, lx;

	u.value = x;
	lx = u.bits;
	hx = u.bits >> 32;

	hx &= 0x7fffffff;
	hx |= (unsigned long) (lx | (-lx)) >> 31;
	hx = 0x7ff00000 - hx;
	return (int) (((unsigned long) hx) >> 31);
}
