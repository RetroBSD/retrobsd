/*
 * Written by Serge Vakulenko <serge@vak.ru>.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 */
#include <math.h>

/*
 * modff(float x, float *iptr)
 * return fraction part of x, and return x's integral part in *iptr.
 */
static const float one = 1.0;

float modff (float x, float *iptr)
{
        unsigned hx = *(unsigned*) &x & ~0x80000000;
        unsigned s;

        if (hx >= 0x4b000000) { /* x is NaN, infinite, or integral */
                *iptr = x;
                if (hx <= 0x7f800000)
                        *(unsigned*) &x &= 0x80000000;
                return x;
        }

        if (hx < 0x3f800000) {  /* |x| < 1 */
                *iptr = x;
                *(unsigned*) iptr &= 0x80000000;
                return x;
        }

        /* split x at the binary point */
        s = *(unsigned*) &x & 0x80000000;
        *(unsigned*) iptr = *(unsigned*) &x & ~((1 << (0x96 - (hx >> 23))) - 1);
        x -= *iptr;

        /* restore sign in case difference is 0 */
        *(unsigned*) &x = (*(unsigned*) &x & ~0x80000000) | s;
        return x;
}

/*
 * For PIC32, double is the same as float.
 */
//double modf (double x, double *iptr) __attribute__((alias ("modff")));
