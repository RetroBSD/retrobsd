/*
 * Written by Serge Vakulenko <serge@vak.ru>.
 *
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 */
#include <math.h>

/*
 * isnan(x) returns 1 is x is nan, else 0;
 * no branching!
 */
int isnanf (float x)
{
	long lx = *(long*) &x;

	lx = 0x7f800000 - (lx & 0x7fffffff);
	return (int) (((unsigned long) lx) >> 31);
}

/*
 * For PIC32, double is the same as float.
 */
int isnan (double x) __attribute__((alias ("isnanf")));
