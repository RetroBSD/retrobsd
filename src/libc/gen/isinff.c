/*
 * Written by Serge Vakulenko <serge@vak.ru>.
 *
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 */
#include <math.h>

/*
 * isinff(x) returns 1 is x is inf, -1 if x is -inf, else 0;
 * no branching!
 */
int isinff (float x)
{
	long lx = *(long*) &x;
	long v = (lx & 0x7fffffff) ^ 0x7f800000;
	return ~((v | -v) >> 31) & (lx >> 30);
}

/*
 * For PIC32, double is the same as float.
 */
int isinf (double x) __attribute__((alias ("isinff")));
