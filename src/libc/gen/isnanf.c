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
        union {
                long s32;
                float f32;
        } u;
        unsigned long ul;

        u.f32 = x;
	ul = 0x7f800000 - (u.s32 & 0x7fffffff);
	return ul >> 31;
}

/*
 * For PIC32, double is the same as float.
 */
int isnan (double x) __attribute__((alias ("isnanf")));
