/*
 * Written by Serge Vakulenko <serge@vak.ru>.
 *
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 */
#include <math.h>

/*
 * modff(float x, float *iptr)
 * return fraction part of x, and return x's integral part in *iptr.
 */
float modff (float fx, float *iptr)
{
        union {
                unsigned u32;
                float f32;
        } x;
        unsigned hx, s;

        x.f32 = fx;
        hx = x.u32 & ~0x80000000;
        if (hx >= 0x4b000000) { /* x is NaN, infinite, or integral */
                *iptr = x.f32;
                if (hx <= 0x7f800000)
                        x.u32 &= 0x80000000;
                return x.f32;
        }

        if (hx < 0x3f800000) {  /* |x| < 1 */
                float ret = x.f32;
                x.u32 &= 0x80000000;
                *iptr = x.f32;
                return ret;
        }

        /* split x at the binary point */
        s = x.u32 & 0x80000000;
        fx = x.f32;
        x.u32 &= ~((1 << (0x96 - (hx >> 23))) - 1);
        *iptr = x.f32;
        x.f32 = fx - *iptr;

        /* restore sign in case difference is 0 */
        x.u32 = (x.u32 & ~0x80000000) | s;
        return x.f32;
}
