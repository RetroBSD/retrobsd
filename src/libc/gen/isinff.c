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
        union {
                long s32;
                float f32;
        } u;
        long v;

        u.f32 = x;
	v = (u.s32 & 0x7fffffff) ^ 0x7f800000;
	return ~((v | -v) >> 31) & (u.s32 >> 30);
}
