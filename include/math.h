/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

extern	double	fabs(), floor(), ceil(), fmod(), ldexp();
extern	double	sqrt(), hypot(), atof();
extern	double	sin(), cos(), tan(), asin(), acos(), atan(), atan2();
extern	double	exp(), log(), log10(), pow();
extern	double	sinh(), cosh(), tanh();
extern	double	gamma();
extern	double	j0(), j1(), jn(), y0(), y1(), yn();

#define	HUGE	1.701411733192644270e38
#define	LOGHUGE	39

int isnanf(float x);
int isnan(double x);

int isinff(float x);
int isinf(double x);

float modff(float x, float *iptr);
double modf(double x, double *iptr);

float frexpf(float x, int *exp);
double frexp(double x, int *exp);

float ldexpf(float x, int exp);
double ldexp(double x, int exp);

double fmod(double x, double y);
