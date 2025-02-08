/*
 * sqrt(a^2 + b^2)
 *	(but carefully)
 */
#include <math.h>

double
hypot(double a, double b)
{
	double t;

	if (a < 0)
                a = -a;
	if (b < 0)
                b = -b;
	if (a > b) {
		t = a;
		a = b;
		b = t;
	}
	if (b==0)
                return(0.);
	a /= b;
	/*
	 * pathological overflow possible
	 * in the next line.
	 */
	return(b*sqrt(1. + a*a));
}

#if 0
struct	complex
{
	double	r;
	double	i;
};

double
cabs(arg)
        struct complex arg;
{
	return(hypot(arg.r, arg.i));
}
#endif
