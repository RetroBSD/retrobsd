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
	long hx, lx;

        lx = *(unsigned long long*) &x;
        hx = (*(unsigned long long*) &x) >> 32;

	lx |= (hx & 0x7fffffff) ^ 0x7ff00000;
	lx |= -lx;
	return ~(lx >> 31) & (hx >> 30);
}
