/*
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 */
#include <math.h>
#include <ieee.h>

/*
 * modf(double x, double *iptr)
 * return fraction part of x, and return x's integral part in *iptr.
 * Method:
 *	Bit twiddling.
 *
 * Exception:
 *	No exception.
 */
static const double one = 1.0;

double modf (double x, double *iptr)
{
	long i0, i1, j0;
	unsigned long i;

	EXTRACT_WORDS (i0, i1, x);
    //
	j0 = ((i0 >> 20) & 0x7ff) - 0x3ff;	/* exponent of x */
	if (j0 < 20) {				/* integer part in high x */
		if (j0 < 0) {			/* |x|<1 */
			INSERT_WORDS (*iptr, i0 & 0x80000000, 0);
            //
			/* *iptr = +-0 */
			return x;
		} else {
			i = (0x000fffff) >> j0;
			if (((i0 & i) | i1) == 0) {	/* x is integral */
				*iptr = x;
				INSERT_WORDS (x, i0 & 0x80000000, 0);
                //
				/* return +-0 */
				return x;
			} else {
				INSERT_WORDS (*iptr, i0 & (~i), 0);
                //
				return x - *iptr;
			}
		}
	} else if (j0 > 51) {			/* no fraction part */
		*iptr = x * one;
		/* We must handle NaNs separately.  */
		if (j0 == 0x400 && ((i0 & 0xfffff) | i1))
			return x * one;

		INSERT_WORDS (x, i0 & 0x80000000, 0);
        //
		/* return +-0 */
		return x;
	} else {				/* fraction part in low x */
		i = ((unsigned long) (0xffffffff)) >> (j0 - 20);
		if ((i1 & i) == 0) {			/* x is integral */
			*iptr = x;
			INSERT_WORDS (x, i0 & 0x80000000, 0);
            //
			/* return +-0 */
			return x;
		} else {
			INSERT_WORDS (*iptr, i0, i1 & (~i));
            //
			return x - *iptr;
		}
	}
}
