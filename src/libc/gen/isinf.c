/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Changed to return -1 for -Inf by Ulrich Drepper <drepper@cygnus.com>.
 * Public domain.
 */
#include <math.h>

/*
 * isinf(x) returns 1 is x is inf, -1 if x is -inf, else 0;
 * no branching!
 */
int isinf (double x)
{
	union {
		double value;
		unsigned long long bits;
	} u;
	long hx, lx;

	u.value = x;
	lx = u.bits;
	hx = u.bits >> 32;

	lx |= (hx & 0x7fffffff) ^ 0x7ff00000;
	lx |= -lx;
	return ~(lx >> 31) & (hx >> 30);
}
