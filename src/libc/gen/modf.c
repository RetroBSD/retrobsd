/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */
// *** modified by Pito 11/2013 for RetroBSD ***

typedef union        // LITTLE ENDIAN
{
  double value;
  struct
  {
    unsigned long lsw;
    unsigned long msw;
  } parts;
} ieee_double_shape_type;

/* Get two 32 bit ints from a double.  */

#define EXTRACT_WORDS(ix0,ix1,d)            \
do {                        \
  ieee_double_shape_type ew_u;               \
  ew_u.value = (d);                  \
  (ix0) = ew_u.parts.msw;               \
  (ix1) = ew_u.parts.lsw;               \
} while (0)

/* Get the more significant 32 bit int from a double.  */

#define GET_HIGH_WORD(i,d)               \
do {                        \
  ieee_double_shape_type gh_u;               \
  gh_u.value = (d);                  \
  (i) = gh_u.parts.msw;                  \
} while (0)

/* Set a double from two 32 bit ints.  */

#define INSERT_WORDS(d,ix0,ix1)               \
do {                        \
  ieee_double_shape_type iw_u;               \
  iw_u.parts.msw = (ix0);               \
  iw_u.parts.lsw = (ix1);               \
  (d) = iw_u.value;                  \
} while (0)

double modf(double x, double *iptr)
{
   long int i0,i1,j0;
   unsigned long i;
   double one = 1.0;
   EXTRACT_WORDS(i0,i1,x);
   j0 = ((i0>>20)&0x7ff)-0x3ff;   /* exponent of x */
   if(j0<20) {         /* integer part in high x */
       if(j0<0) {         /* |x|<1 */
      INSERT_WORDS(*iptr,i0&0x80000000,0);   /* *iptr = +-0 */
      return x;
       } else {
      i = (0x000fffff)>>j0;
      if(((i0&i)|i1)==0) {      /* x is integral */
          unsigned long high;
          *iptr = x;
          GET_HIGH_WORD(high,x);
          INSERT_WORDS(x,high&0x80000000,0);   /* return +-0 */
          return x;
      } else {
          INSERT_WORDS(*iptr,i0&(~i),0);
          return x - *iptr;
      }
       }
   } else if (j0>51) {      /* no fraction part */
       unsigned long high;
       *iptr = x*one;
       GET_HIGH_WORD(high,x);
       INSERT_WORDS(x,high&0x80000000,0);   /* return +-0 */
       return x;
   } else {         /* fraction part in low x */
       i = ((unsigned long)(0xffffffff))>>(j0-20);
       if((i1&i)==0) {       /* x is integral */
      unsigned long high;
      *iptr = x;
      GET_HIGH_WORD(high,x);
      INSERT_WORDS(x,high&0x80000000,0);   /* return +-0 */
      return x;
       } else {
      INSERT_WORDS(*iptr,i0,i1&(~i));
      return x - *iptr;
       }
   }
}
