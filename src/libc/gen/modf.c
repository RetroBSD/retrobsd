/*
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 */
#include <math.h>

// IM: dereferencing type-punned pointer will break strict-aliasing rules
// TODO check Endiannes
        union {
              double d64;
              struct { long i32h;
                       long i32l;
                     };
        } ew;

/* Get two 32 bit ints from a double.  */

//#define EXTRACT_WORDS(high,low,d) 
//        high = *(unsigned long long*) &d; 
//        low  = (*(unsigned long long*) &d) >> 32

/* Set a double from two 32 bit ints.  */

#define INSERT_WORDS(d,high,low) \
        *(unsigned long long*) &(x) = (unsigned long long) (high) << 32 | (low)

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

	//EXTRACT_WORDS (i0, i1, x);
    ew.d64 = x; i0 = ew.i32h; i1 = ew.i32l;
    //
	j0 = ((i0 >> 20) & 0x7ff) - 0x3ff;	/* exponent of x */
	if (j0 < 20) {				/* integer part in high x */
		if (j0 < 0) {			/* |x|<1 */
			//INSERT_WORDS (*iptr, i0 & 0x80000000, 0);
            ew.i32h = i0 & 0x80000000; ew.i32l = 0; *iptr = ew.d64;
            //
			/* *iptr = +-0 */
			return x;
		} else {
			i = (0x000fffff) >> j0;
			if (((i0 & i) | i1) == 0) {	/* x is integral */
				*iptr = x;
				//INSERT_WORDS (x, i0 & 0x80000000, 0);
                ew.i32h = i0 & 0x80000000; ew.i32l = 0; x = ew.d64;
                //
				/* return +-0 */
				return x;
			} else {
				//INSERT_WORDS (*iptr, i0 & (~i), 0);
                ew.i32h = i0 & (~i); ew.i32l = 0; *iptr = ew.d64;
                //
				return x - *iptr;
			}
		}
	} else if (j0 > 51) {			/* no fraction part */
		*iptr = x * one;
		/* We must handle NaNs separately.  */
		if (j0 == 0x400 && ((i0 & 0xfffff) | i1))
			return x * one;

		//INSERT_WORDS (x, i0 & 0x80000000, 0);
        ew.i32h = i0 & 0x80000000; ew.i32l = 0; x = ew.d64;
        //
		/* return +-0 */
		return x;
	} else {				/* fraction part in low x */
		i = ((unsigned long) (0xffffffff)) >> (j0 - 20);
		if ((i1 & i) == 0) {			/* x is integral */
			*iptr = x;
			//INSERT_WORDS (x, i0 & 0x80000000, 0);
            ew.i32h = i0 & 0x80000000; ew.i32l = 0; x = ew.d64;
            //
			/* return +-0 */
			return x;
		} else {
			//INSERT_WORDS (*iptr, i0, i1 & (~i));
            ew.i32h = i0; ew.i32l = i1 & (~i); *iptr = ew.d64;
            //
			return x - *iptr;
		}
	}
}
