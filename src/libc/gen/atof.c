/*
 * C library - ascii to floating
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <float.h>
#include <math.h>
#include <ctype.h>

/*
 * BIG = 2**(DBL_MANT_DIG+3) defines how many decimal digits
 * to take into account from the input. It doesn't make sense
 * to use more digits than log10(2**DBL_MANT_DIG)+1.
 * BIG is equal 2**27 or 2**56, depending on whether double
 * is single or double precision.
 */
#define BIG (8 * (double)(1L << (DBL_MANT_DIG/2)) * \
        (double)(1L << (DBL_MANT_DIG/2 + DBL_MANT_DIG%2)))

double
atof(p)
register char *p;
{
	register int c;
	double fl, flexp, exp5;
	double big = BIG;
	int nd;
	register int eexp, exp, neg, negexp, bexp;

	neg = 1;
	while((c = *p++) == ' ')
		;
	if (c == '-')
		neg = -1;
	else if (c=='+')
		;
	else
		--p;

	exp = 0;
	fl = 0;
	nd = 0;
	while ((c = *p++), isdigit(c)) {
		if (fl<big)
			fl = 10*fl + (c-'0');
		else
			exp++;
		nd++;
	}

	if (c == '.') {
		while ((c = *p++), isdigit(c)) {
			if (fl<big) {
				fl = 10*fl + (c-'0');
				exp--;
			}
		nd++;
		}
	}

	negexp = 1;
	eexp = 0;
	if ((c == 'E') || (c == 'e')) {
		if ((c= *p++) == '+')
			;
		else if (c=='-')
			negexp = -1;
		else
			--p;

		while ((c = *p++), isdigit(c)) {
			eexp = 10*eexp+(c-'0');
		}
		if (negexp<0)
			eexp = -eexp;
		exp = exp + eexp;
	}

	negexp = 1;
	if (exp<0) {
		negexp = -1;
		exp = -exp;
	}


	if ((nd+exp*negexp) < DBL_MIN_10_EXP - 2) {
		fl = 0;
		exp = 0;
	}
	flexp = 1;
	exp5 = 5;
	bexp = exp;
	for (;;) {
		if (exp&01)
			flexp *= exp5;
		exp >>= 1;
		if (exp==0)
			break;
		exp5 *= exp5;
	}
	if (negexp<0)
		fl /= flexp;
	else
		fl *= flexp;
	fl = ldexp(fl, negexp*bexp);
	if (neg<0)
		fl = -fl;
	return(fl);
}
